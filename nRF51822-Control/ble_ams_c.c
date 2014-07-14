#include "ble_ams_c.h"
#include <string.h>
#include <stdbool.h>
#include "ble_err.h"
#include "ble_srv_common.h"
#include "nordic_common.h"
#include "nrf_assert.h"
#include "device_manager.h"
#include "ble_flash.h"
#include "pstorage.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "led.h"

#define START_HANDLE_DISCOVER            0x0001
#define BLE_AMS_MAX_DISCOVERED_CENTRALS  DEVICE_MANAGER_MAX_BONDS
#define DISCOVERED_SERVICE_DB_SIZE \
    CEIL_DIV(sizeof(apple_service_t) * BLE_AMS_MAX_DISCOVERED_CENTRALS, sizeof(uint32_t))

#define TX_BUFFER_MASK                   0x07                                              /**< TX Buffer mask, must be a mask of contiguous zeroes, followed by contiguous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE                   (TX_BUFFER_MASK + 1)                              /**< Size of send buffer, which is 1 higher than the mask. */
#define WRITE_MESSAGE_LENGTH             20                                                /**< Length of the write message for CCCD/remote command. */
#define NOTIFICATION_DATA_LENGTH         2                                                 /**< The mandatory length of notification data. After the mandatory data, the optional message is located. */

typedef enum
{
    READ_REQ = 1,                                                                          /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ                                                                              /**< Type identifying that this tx_message is a write request. */
} ams_tx_request_t;

typedef enum
{
    STATE_UNINITIALIZED,                                                                   /**< Uninitialized state of the internal state machine. */
    STATE_IDLE,                                                                            /**< Idle state, this is the state when no master has connected to this device. */
    STATE_DISC_SERV,                                                                       /**< A BLE master is connected and a service discovery is in progress. */
    STATE_DISC_CHAR,                                                                       /**< A BLE master is connected and characteristic discovery is in progress. */
    STATE_DISC_DESC,                                                                       /**< A BLE master is connected and descriptor discovery is in progress. */
    STATE_RUNNING,                                                                         /**< A BLE master is connected and complete service discovery has been performed. */
    STATE_WAITING_ENC,                                                                     /**< A previously bonded BLE master has re-connected and the service awaits the setup of an encrypted link. */
    STATE_RUNNING_NOT_DISCOVERED,                                                          /**< A BLE master is connected and a service discovery is in progress. */
} ams_state_t;

/* brief Structure used for holding the characteristic found during discovery process.
 */
typedef struct
{
    ble_uuid_t               uuid;                                                         /**< UUID identifying this characteristic. */
    ble_gatt_char_props_t    properties;                                                   /**< Properties for this characteristic. */
    uint16_t                 handle_decl;                                                  /**< Characteristic Declaration Handle for this characteristic. */
    uint16_t                 handle_value;                                                 /**< Value Handle for the value provided in this characteristic. */
    uint16_t                 handle_cccd;                                                  /**< CCCD Handle value for this characteristic. BLE_AMS_INVALID_HANDLE if not present in the master. */
} apple_characteristic_t;

/**@brief Structure used for holding the Apple Media Service found during discovery process.
 */
typedef struct
{
    uint8_t                  handle;                                                       /**< Handle of Apple Media Service which identifies to which master this discovered service belongs. */
    ble_gattc_service_t      service;                                                      /**< The GATT service holding the discovered Apple Media Service. */
    apple_characteristic_t   remote_command;
    apple_characteristic_t   entity_update;
    apple_characteristic_t   entity_attribute;
} apple_service_t;

/**@brief Structure for writing a message to the master, i.e. Remote Command or CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];                            /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                                                 /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected master.
 */
typedef struct
{
    uint16_t                 conn_handle;                                                  /**< Connection handle to be used when transmitting this message. */
    ams_tx_request_t         type;                                                         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t             read_handle;                                                  /**< Read request message. */
        write_params_t       write_req;                                                    /**< Write request message. */
    } req;
} tx_message_t;

static tx_message_t          m_tx_buffer[TX_BUFFER_SIZE];                                  /**< Transmit buffer for messages to be transmitted to the master. */
static uint32_t              m_tx_insert_index = 0;                                        /**< Current index in the transmit buffer where next message should be inserted. */
static uint32_t              m_tx_index = 0;                                               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */
static pstorage_handle_t     m_flash_handle;                                               /**< Flash handle where discovered services for bonded masters should be stored. */

static ams_state_t           m_client_state = STATE_UNINITIALIZED;                          /**< Current state of the Apple Media State Machine. */

static uint32_t              m_service_db[DISCOVERED_SERVICE_DB_SIZE];                     /**< Service database for bonded masters (Word size aligned). */
static apple_service_t *     mp_service_db;                                                /**< Pointer to start of discovered services database. */
static apple_service_t       m_service;                                                    /**< Current service data. */

static ble_ams_c_t *         m_ams_c_obj;                                                 /**< Pointer to the instantiated object. */

const ble_uuid128_t ble_ams_base_uuid128 =
{
    {
        // 89D3502B-0F36-433A-8EF4-C502AD55F8DC
        0xdc, 0xf8, 0x55, 0xad, 0x02, 0xc5, 0xf4, 0x8e,
        0x3a, 0x43, 0x36, 0x0f, 0x2b, 0x50, 0xd3, 0x89
        
    }
};

const ble_uuid128_t ble_ams_rc_base_uuid128 =
{
    {
        // 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2
        0xc2, 0x51, 0xca, 0xf7, 0x56, 0x0e, 0xdf, 0xb8,
        0x8a, 0x4a, 0xb1, 0x57, 0xd8, 0x81, 0x3c, 0x9b
        
    }
};

const ble_uuid128_t ble_ams_eu_base_uuid128 =
{
    {
        // 2F7CABCE-808D-411F-9A0C-BB92BA96C102
        0x02, 0xc1, 0x96, 0xba, 0x92, 0xbb, 0x0c, 0x9a,
        0x1f, 0x41, 0x8d, 0x80, 0xce, 0xab, 0x7c, 0x2f
        
    }
};

const ble_uuid128_t ble_ams_ea_base_uuid128 =
{
    {
        // C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7
        0xd7, 0xd5, 0xbb, 0x70, 0xa8, 0xa3, 0xab, 0xa6,
        0xd8, 0x46, 0xab, 0x23, 0x8c, 0xf3, 0xb2, 0xc6
        
    }
};

/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;
        
        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            ++m_tx_index;
            m_tx_index &= TX_BUFFER_MASK;
        }
    }
}

/**@brief Function for updating the current state and sending an event on discovery failure.
*/
static void handle_discovery_failure(const ble_ams_c_t * p_ams, uint32_t code)
{
    ble_ams_c_evt_t event;
    
    m_client_state        = STATE_RUNNING_NOT_DISCOVERED;
    event.evt_type        = BLE_AMS_C_EVT_DISCOVER_FAILED;
    event.data.error_code = code;
    
    p_ams->evt_handler(&event);
}

/**@brief Function for executing the Service Discovery Procedure.
 */
static void service_disc_req_send(const ble_ams_c_t * p_ams)
{
    uint16_t   handle = START_HANDLE_DISCOVER;
    ble_uuid_t ams_uuid;
    uint32_t   err_code;
    
    // Discover services on uuid for ANCS.
    BLE_UUID_BLE_ASSIGN(ams_uuid, BLE_UUID_APPLE_MEDIA_SERVICE);
    ams_uuid.type = BLE_UUID_TYPE_VENDOR_BEGIN;
    
    err_code = sd_ble_gattc_primary_services_discover(p_ams->conn_handle, handle, &ams_uuid);
    if (err_code != NRF_SUCCESS)
    {
        handle_discovery_failure(p_ams, err_code);
    }
    else
    {
        m_client_state = STATE_DISC_SERV;
    }
}

/**@brief Function for executing the Characteristic Discovery Procedure.
 */
static void characteristic_disc_req_send(const ble_ams_c_t * p_ams,
                                         const ble_gattc_handle_range_t * p_handle)
{
    uint32_t err_code;
    
    // With valid service, we should discover characteristics.
    err_code = sd_ble_gattc_characteristics_discover(p_ams->conn_handle, p_handle);
    
    if (err_code == NRF_SUCCESS)
    {
        m_client_state = STATE_DISC_CHAR;
    }
    else
    {
        handle_discovery_failure(p_ams, err_code);
    }
}

/**@brief Function for executing the Characteristic Descriptor Discovery Procedure.
 */
static void descriptor_disc_req_send(const ble_ams_c_t * p_ams)
{
    ble_gattc_handle_range_t descriptor_handle;
    uint32_t                 err_code = NRF_SUCCESS;
    
    // If we have not discovered descriptors for all characteristics,
    // we will discover next descriptor.
    if (m_service.entity_update.handle_cccd == BLE_AMS_INVALID_HANDLE)
    {
        descriptor_handle.start_handle = m_service.entity_update.handle_value + 1;
        descriptor_handle.end_handle = m_service.entity_update.handle_value + 1;
        
        err_code = sd_ble_gattc_descriptors_discover(p_ams->conn_handle, &descriptor_handle);
    }
    else if (m_service.remote_command.handle_cccd == BLE_AMS_INVALID_HANDLE)
    {
        descriptor_handle.start_handle = m_service.remote_command.handle_value + 1;
        descriptor_handle.end_handle = m_service.remote_command.handle_value + 1;
        
        err_code = sd_ble_gattc_descriptors_discover(p_ams->conn_handle, &descriptor_handle);
    }
    
    
    if (err_code == NRF_SUCCESS)
    {
        m_client_state = STATE_DISC_DESC;
    }
    else
    {
        handle_discovery_failure(p_ams, err_code);
    }
}

/**@brief Function for indicating that a connection has successfully been established.
 *        Either when the Service Discovery Procedure completes or a re-connection has been
 *        established to a bonded master.
 *
 * @details This function is executed when a service discovery or a re-connect to a bonded master
 *          occurs. In the event of re-connection to a bonded master, this function will ensure
 *          writing of the control point according to the Apple Media Service Client
 *          specification.
 *          Finally an event is passed to the application:
 *          BLE_AMS_C_EVT_RECONNECT         - When we are connected to an existing master and the
 *                                            apple notification control point has been written.
 *          BLE_AMS_C_EVT_DISCOVER_COMPLETE - When we are connected to a new master and the Service
 *                                            Discovery has been completed.
 */
static void connection_established(const ble_ams_c_t * p_ams)
{
    ble_ams_c_evt_t event;
    
    m_client_state = STATE_RUNNING;
    
    event.evt_type = BLE_AMS_C_EVT_DISCOVER_COMPLETE;
    p_ams->evt_handler(&event);
}

/**@brief Function for waiting until an encrypted link has been established to a bonded master.
 */
static void encrypted_link_setup_wait(const ble_ams_c_t * p_ams)
{
    m_client_state = STATE_WAITING_ENC;
}

/**@brief Function for handling the connect event when a master connects.
 *
 * @details This function will check if bonded master connects, and do the following
 *          Bonded master  - enter wait for encryption state.
 *          Unknown master - Initiate service discovery procedure.
 */
static void event_connect(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    p_ams->conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    
    if (p_ams->central_handle != DM_INVALID_ID)
    {
        m_service = mp_service_db[p_ams->central_handle];
        encrypted_link_setup_wait(p_ams);
    }
    else
    {
        m_service.handle = INVALID_SERVICE_HANDLE;
        service_disc_req_send(p_ams);
    }
}

/**@brief Function for handling the encrypted link event when a secure
 *        connection has been established with a master.
 *
 * @details This function will check if the service for the master is known.
 *          Service known   - Execute action running / switch to running state.
 *          Service unknown - Initiate Service Discovery Procedure.
 */
static void event_encrypted_link(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    // If we are setting up a bonded connection and the UUID of the service
    // is unknown, a new discovery must be performed.
    if (m_service.service.uuid.uuid != BLE_UUID_APPLE_MEDIA_SERVICE)
    {
        m_service.handle = INVALID_SERVICE_HANDLE;
        service_disc_req_send(p_ams);
    }
    else
    {
        connection_established(p_ams);
    }
}

/**@brief Function for handling the response on service discovery.
 *
 * @details This function will validate the response.
 *          Invalid response - Handle discovery failure.
 *          Valid response   - Initiate Characteristic Discovery Procedure.
 */
static void event_discover_rsp(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status != BLE_GATT_STATUS_SUCCESS)
    {
        // We have received an  unexpected result.
        // As we are in a connected state, but can not continue
        // our service discovery, we will go to running state.
        handle_discovery_failure(p_ams, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        BLE_UUID_COPY_INST(m_service.service.uuid,
                           p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[0].uuid);
        
        if (p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.count > 0)
        {
            const ble_gattc_service_t * p_service;
            
            p_service = &(p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[0]);
            
            m_service.service.handle_range.start_handle = p_service->handle_range.start_handle;
            m_service.service.handle_range.end_handle   = p_service->handle_range.end_handle;
            
            characteristic_disc_req_send(p_ams, &(m_service.service.handle_range));
        }
        else
        {
            // If we receive an answer, but it contains no service, we just go to state: Running.
            handle_discovery_failure(p_ams, p_ble_evt->evt.gattc_evt.gatt_status);
        }
    }
}

/**@brief Function for setting the discovered characteristics in the apple service.
 */
static void characteristics_set(apple_characteristic_t * p_characteristic,
                                const ble_gattc_char_t * p_char_resp)
{
    BLE_UUID_COPY_INST(p_characteristic->uuid, p_char_resp->uuid);
    
    p_characteristic->properties   = p_char_resp->char_props;
    p_characteristic->handle_decl  = p_char_resp->handle_decl;
    p_characteristic->handle_value = p_char_resp->handle_value;
    p_characteristic->handle_cccd  = BLE_AMS_INVALID_HANDLE;
}

/**@brief Function for handling a characteristic discovery response event.
 *
 * @details This function will validate and store the characteristics received.
 *          If not all characteristics are discovered it will continue the characteristic discovery
 *          procedure, otherwise it will continue with the descriptor discovery procedure.
 *          If we receive a GATT Client error, we will go to handling of discovery failure.
 */
static void event_characteristic_rsp(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND ||
        p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INVALID_HANDLE)
    {
        if ((m_service.remote_command.handle_value == 0)    ||
            (m_service.entity_update.handle_value == 0)    ||
            (m_service.entity_attribute.handle_value == 0) )
        {
            // At least one required characteristic is missing on the server side.
            handle_discovery_failure(p_ams, NRF_ERROR_NOT_FOUND);
        }
        else
        {
            connection_established(p_ams);
            //descriptor_disc_req_send(p_ams);
        }
    }
    else if (p_ble_evt->evt.gattc_evt.gatt_status)
    {
        // We have received an  unexpected result.
        // As we are in a connected state, but can not continue
        // our service discovery, we must go to running state.
        handle_discovery_failure(p_ams, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        uint32_t                 i;
        const ble_gattc_char_t * p_char_resp = NULL;
        
        // Iterate trough the characteristics and find the correct one.
        for (i = 0; i < p_ble_evt->evt.gattc_evt.params.char_disc_rsp.count; i++)
        {
            p_char_resp = &(p_ble_evt->evt.gattc_evt.params.char_disc_rsp.chars[i]);
            switch (p_char_resp->uuid.uuid)
            {
                case BLE_UUID_AMS_REMOTE_COMMAND_CHAR:
                    characteristics_set(&m_service.remote_command, p_char_resp);
                    break;
                    
                case BLE_UUID_AMS_ENTITY_UPDATE_CHAR:
                    characteristics_set(&m_service.entity_update, p_char_resp);
                    break;
                    
                case BLE_UUID_AMS_ENTITY_ATTRIBUTE_CHAR:
                    characteristics_set(&m_service.entity_attribute, p_char_resp);
                    break;
                    
                default:
                    break;
            }
        }
        
        
        if (p_char_resp != NULL)
        {
            // If not all characteristics are received, we do a 2nd/3rd/... round.
            ble_gattc_handle_range_t char_handle;
            
            char_handle.start_handle = p_char_resp->handle_value + 1;
            char_handle.end_handle   = m_service.service.handle_range.end_handle;
            
            characteristic_disc_req_send(p_ams, &char_handle);
        }
        else
        {
            characteristic_disc_req_send(p_ams, &(m_service.service.handle_range));
        }
    }
}

/**@brief Function for setting the discovered descriptor in the apple service.
 */
static void descriptor_set(apple_service_t * p_service, const ble_gattc_desc_t * p_desc_resp)
{
    if (p_service->remote_command.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->remote_command.handle_cccd = p_desc_resp->handle;
        }
    }
    
    else if (p_service->entity_update.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->entity_update.handle_cccd = p_desc_resp->handle;
        }
    }
    else if (p_service->entity_attribute.handle_value == (p_desc_resp->handle - 1))
    {
        if (p_desc_resp->uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
        {
            p_service->entity_attribute.handle_cccd = p_desc_resp->handle;
        }
    }
}

/**@brief Function for handling of descriptor discovery responses.
 *
 * @details This function will validate and store the descriptor received.
 *          If not all descriptors are discovered it will continue the descriptor discovery
 *          procedure, otherwise it will continue to running state.
 *          If we receive a GATT Client error, we will go to handling of discovery failure.
 */
static void event_descriptor_rsp(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    if (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND ||
        p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INVALID_HANDLE)
    {
        handle_discovery_failure(p_ams, NRF_ERROR_NOT_FOUND);
    }
    else if (p_ble_evt->evt.gattc_evt.gatt_status)
    {
        // We have received an unexpected result.
        // As we are in a connected state, but can not continue
        // our descriptor discovery, we will go to running state.
        handle_discovery_failure(p_ams, p_ble_evt->evt.gattc_evt.gatt_status);
    }
    else
    {
        if (p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.count > 0)
        {
            descriptor_set(&m_service, &(p_ble_evt->evt.gattc_evt.params.desc_disc_rsp.descs[0]));
        }
        
        if (m_service.remote_command.handle_cccd == BLE_AMS_INVALID_HANDLE ||
            m_service.entity_update.handle_cccd == BLE_AMS_INVALID_HANDLE)
        {
            descriptor_disc_req_send(p_ams);
        }
        else
        {
            connection_established(p_ams);
        }
    }
}

/**@brief Function for handling write response events.
 */
static void event_write_rsp(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    tx_buffer_process();
}

/**@brief Function for disconnecting and cleaning the current service.
 */
static void event_disconnect(ble_ams_c_t * p_ams)
{
    m_client_state = STATE_IDLE;
    
    if (m_service.handle == INVALID_SERVICE_HANDLE_DISC &&
        p_ams->central_handle != DM_INVALID_ID)
    {
        m_service.handle = p_ams->central_handle;
    }
    
    if (m_service.handle < BLE_AMS_MAX_DISCOVERED_CENTRALS)
    {
        mp_service_db[m_service.handle] = m_service;
    }
    
    memset(&m_service, 0, sizeof(apple_service_t));
    
    m_service.handle       = INVALID_SERVICE_HANDLE;
    p_ams->service_handle = INVALID_SERVICE_HANDLE;
    p_ams->conn_handle    = BLE_CONN_HANDLE_INVALID;
    p_ams->central_handle  = DM_INVALID_ID;
}

/**@brief Function for handling of Device Manager events.
 */
void ble_ams_c_on_device_manager_evt(ble_ams_c_t      * p_ams,
                                      dm_handle_t const * p_handle,
                                      dm_event_t const  * p_dm_evt)
{
    switch (p_dm_evt->event_id)
    {
        case DM_EVT_CONNECTION:
            // Fall through.
        case DM_EVT_SECURITY_SETUP_COMPLETE:
            p_ams->central_handle = p_handle->device_id;
            break;
        default:
            // Do nothing.
            break;
    }
}

/**@brief Function for receiving and validating notifications received from the master.
 */
static void event_notify(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    //TBD
}

/**@brief Function for handling of BLE stack events.
 */
void ble_ams_c_on_ble_evt(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt)
{
    uint16_t event = p_ble_evt->header.evt_id;
    
    switch (m_client_state)
    {
        case STATE_UNINITIALIZED:
            // Initialization is handle in special case, thus if we enter here, it means that an
            // event is received even though we are not initialized --> ignore.
            break;
            
        case STATE_IDLE:
            if (event == BLE_GAP_EVT_CONNECTED)
            {
                event_connect(p_ams, p_ble_evt);
            }
            break;
            
        case STATE_WAITING_ENC:
            if ((event == BLE_GAP_EVT_AUTH_STATUS)|| (event == BLE_GAP_EVT_SEC_INFO_REQUEST))
            {
                event_encrypted_link(p_ams, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            break;
            
        case STATE_DISC_SERV:
            if (event == BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP)
            {
                event_discover_rsp(p_ams, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            break;
            
        case STATE_DISC_CHAR:
            if (event == BLE_GATTC_EVT_CHAR_DISC_RSP)
            {
                event_characteristic_rsp(p_ams, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            break;
            
        case STATE_DISC_DESC:
            if (event == BLE_GATTC_EVT_DESC_DISC_RSP)
            {
                event_descriptor_rsp(p_ams, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            break;
            
        case STATE_RUNNING:
            if (event == BLE_GATTC_EVT_HVX)
            {
                event_notify(p_ams, p_ble_evt);
            }
            else if (event == BLE_GATTC_EVT_WRITE_RSP)
            {
                event_write_rsp(p_ams, p_ble_evt);
            }
            else if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            else
            {
                // Do nothing, event not handled in this state.
            }
            break;
            
        case STATE_RUNNING_NOT_DISCOVERED:
        default:
            if (event == BLE_GAP_EVT_DISCONNECTED)
            {
                event_disconnect(p_ams);
            }
            break;
    }
}

static void ams_pstorage_callback(pstorage_handle_t * handle,
                                   uint8_t             op_code,
                                   uint32_t            reason,
                                   uint8_t           * p_data,
                                   uint32_t            param_len)
{
    if (reason != NRF_SUCCESS)
    {
        m_ams_c_obj->error_handler(reason);
    }
}

uint32_t ble_ams_c_init(ble_ams_c_t * p_ams, const ble_ams_c_init_t * p_ams_init)
{
    uint32_t                err_code;
    pstorage_module_param_t param;
    
    if (p_ams_init->evt_handler == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    
    p_ams->evt_handler         = p_ams_init->evt_handler;
    p_ams->error_handler       = p_ams_init->error_handler;
    p_ams->service_handle      = INVALID_SERVICE_HANDLE;
    p_ams->central_handle       = DM_INVALID_ID;
    p_ams->service_handle      = 0;
    p_ams->message_buffer_size = p_ams_init->message_buffer_size;
    p_ams->p_message_buffer    = p_ams_init->p_message_buffer;
    p_ams->conn_handle         = BLE_CONN_HANDLE_INVALID;
    
    memset(&m_service, 0, sizeof(apple_service_t));
    memset(m_tx_buffer, 0, TX_BUFFER_SIZE);
    
    m_service.handle = INVALID_SERVICE_HANDLE;
    m_client_state   = STATE_IDLE;
    
    param.block_count = 1;
    param.block_size  = DISCOVERED_SERVICE_DB_SIZE * sizeof(uint32_t); // uint32_t array.
    param.cb          = ams_pstorage_callback;
    
    // Register with storage module.
    err_code = pstorage_register(&param, &m_flash_handle);
    
    mp_service_db    = (apple_service_t *) (m_service_db);
    
    return err_code;
}

/**@brief Function for creating a TX message for writing a CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? 0x0001 : 0;
    
    if (m_client_state != STATE_RUNNING)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;
    
    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = 2;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;
    
    tx_buffer_process();
    return NRF_SUCCESS;
}

uint32_t ble_ams_c_enable_notif_remote_control(const ble_ams_c_t * p_ams)
{
    return cccd_configure(p_ams->conn_handle,
                          m_service.remote_command.handle_cccd,
                          true);
}

uint32_t ble_ams_send_rc_command(ble_ams_c_t * p_ams, const ble_ams_remote_command_values_t p_cmd)
{
    tx_message_t * p_msg;
    uint32_t i = 0;
    
    if (m_client_state != STATE_RUNNING)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;
    
    p_msg->req.write_req.gattc_params.handle   = m_service.remote_command.handle_value;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    
    p_msg->req.write_req.gattc_value[i++]      = p_cmd;
    
    p_msg->req.write_req.gattc_params.len      = i;
    p_msg->conn_handle                         = p_ams->conn_handle;
    p_msg->type                                = WRITE_REQ;
    
    tx_buffer_process();
    return NRF_SUCCESS;
}

uint32_t ble_ams_c_service_load(const ble_ams_c_t * p_ams)
{
    uint32_t err_code;
    uint32_t i;
    
    err_code = pstorage_load((uint8_t *)m_service_db,
                             &m_flash_handle,
                             (DISCOVERED_SERVICE_DB_SIZE * sizeof(uint32_t)),
                             0);
    
    if (err_code != NRF_SUCCESS)
    {
        // Problem with loading values from flash, initialize the RAM DB with default.
        for (i = 0; i < BLE_AMS_MAX_DISCOVERED_CENTRALS; ++i)
        {
            mp_service_db[i].handle = INVALID_SERVICE_HANDLE;
        }
        
        if (err_code == NRF_ERROR_NOT_FOUND)
        {
            // The flash does not contain any memorized centrals, set the return code to success.
            err_code = NRF_SUCCESS;
        }
    }
    return err_code;
}


uint32_t ble_ams_c_service_store(void)
{
    uint32_t err_code;
    
    err_code = pstorage_store(&m_flash_handle,
                              (uint8_t *) m_service_db,
                              DISCOVERED_SERVICE_DB_SIZE * sizeof(uint32_t),
                              0);
    
    return err_code;
}


uint32_t ble_ams_c_service_delete(void)
{
    if (m_client_state == STATE_UNINITIALIZED)
    {
        return NRF_SUCCESS;
    }
    
    return pstorage_clear(&m_flash_handle, (DISCOVERED_SERVICE_DB_SIZE * sizeof(uint32_t)));
}