#ifndef BLE_AMS_H__
#define BLE_AMS_H__

#include "ble_types.h"
#include "ble_srv_common.h"
#include "device_manager.h"

#define AMS_NB_OF_CHARACTERISTICS           3
#define AMS_NB_OF_SERVICES                  1

#define INVALID_SERVICE_HANDLE_BASE                 0xF0                                 /**< Base for indicating invalid service handle. */
#define INVALID_SERVICE_HANDLE                      (INVALID_SERVICE_HANDLE_BASE + 0x0F) /**< Indication that the current service handle is invalid. */
#define INVALID_SERVICE_HANDLE_DISC                 (INVALID_SERVICE_HANDLE_BASE + 0x0E) /**< Indication that the current service handle is invalid but the service has been discovered. */
#define BLE_AMS_INVALID_HANDLE                     0xFF                                 /**< Indication that the current service handle is invalid. */
#define AMS_ATTRIBUTE_DATA_MAX                     32                                   /*<< Maximium notification attribute data length. */


#define BLE_UUID_APPLE_MEDIA_SERVICE        0x502B
#define BLE_UUID_AMS_REMOTE_COMMAND_CHAR    0x81D8
#define BLE_UUID_AMS_ENTITY_UPDATE_CHAR     0xABCE
#define BLE_UUID_AMS_ENTITY_ATTRIBUTE_CHAR  0xF38C

typedef struct ble_ams_c_s ble_ams_c_s;

/**@brief Event types that are passed from client to application on an event. */
typedef enum
{
    BLE_AMS_C_EVT_DISCOVER_COMPLETE,          /**< A successful connection has been established and the characteristics of the server has been fetched. */
    BLE_AMS_C_EVT_DISCOVER_FAILED,            /**< It was not possible to discover service or characteristics of the connected peer. */
} ble_ams_c_evt_type_t;

/**@brief Remote Commands for AMS. */
typedef enum
{
    BLE_AMS_REMOTE_COMMAND_PLAY,
    BLE_AMS_REMOTE_COMMAND_PAUSE,
    BLE_AMS_REMOTE_COMMAND_TOGGLE_PLAY_PAUSE,
    BLE_AMS_REMOTE_COMMAND_NEXT_TRACK,
    BLE_AMS_REMOTE_COMMAND_PREV_TRACK,
    BLE_AMS_REMOTE_COMMAND_VOLUME_UP,
    BLE_AMS_REMOTE_COMMAND_VOLUME_DOWN,
    BLE_AMS_REMOTE_COMMAND_REPEAT_MODE,
    BLE_AMS_REMOTE_COMMAND_SHUFFLE_MODE,
    BLE_AMS_REMOTE_COMMAND_SKIP_FORWARD,
    BLE_AMS_REMOTE_COMMAND_SKIP_BACKWARD
} ble_ams_remote_command_values_t;

typedef struct {
    uint8_t                            event_id;
    uint8_t                            event_flags;
    uint8_t                            category_id;
    uint8_t                            category_count;
    uint8_t                            notification_uid[4];
} ble_ams_c_evt_ios_notification_t;

typedef struct {
    uint8_t                            command_id;
    uint8_t                            notification_uid[4];
    uint8_t                            attribute_id;
    uint16_t                           attribute_len;
    uint8_t                            data[AMS_ATTRIBUTE_DATA_MAX];
} ble_ams_c_evt_notif_attribute_t;

/**@brief Apple Media Event structure
 *
 * @details The structure contains the event that should be handled, as well as
 *          additional information.
 */
typedef struct
{
    ble_ams_c_evt_type_t                evt_type;                                         /**< Type of event. */
    ble_uuid_t                          uuid;                                             /**< UUID of the event in case of an apple or notification. */
    union
    {
        ble_ams_c_evt_ios_notification_t   notification;
        ble_ams_c_evt_notif_attribute_t    attribute;
        uint32_t                        error_code;                                       /**< Additional status/error code if the event was caused by a stack error or gatt status, e.g. during service discovery. */
    } data;
} ble_ams_c_evt_t;


/**@brief Apple Media event handler type. */
typedef void (*ble_ams_c_evt_handler_t) (ble_ams_c_evt_t * p_evt);

/**@brief Apple Media structure. This contains various status information for the client. */
typedef struct ble_ams_c_s
{
    ble_ams_c_evt_handler_t             evt_handler;
    ble_srv_error_handler_t             error_handler;
    uint16_t                            conn_handle;
    uint8_t                             central_handle;
    uint8_t                             service_handle;
    uint32_t                            message_buffer_size;
    uint8_t *                           p_message_buffer;
} ble_ams_c_t;

/**@brief Apple Media init structure. This contains all options and data needed for
 *        initialization of the client.*/
typedef struct
{
    ble_ams_c_evt_handler_t             evt_handler;
    ble_srv_error_handler_t             error_handler;
    uint32_t                            message_buffer_size;
    uint8_t *                           p_message_buffer;
} ble_ams_c_init_t;

/**@brief Apple Media Service UUIDs */
extern const ble_uuid128_t ble_ams_base_uuid128;                                         /**< Service UUID. */
extern const ble_uuid128_t ble_ams_rc_base_uuid128;                                      /**< Remote Command UUID. */
extern const ble_uuid128_t ble_ams_eu_base_uuid128;                                      /**< Entity Update UUID. */
extern const ble_uuid128_t ble_ams_ea_base_uuid128;                                      /**< Entity Attribute UUID. */

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the AMS Client.
 *
 * @param[in]   p_ams     AMS Client structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_ams_c_on_ble_evt(ble_ams_c_t * p_ams, const ble_evt_t * p_ble_evt);

/**@brief Function for handling the AMS Client - Device Manager event.
 *
 * @details Handles all events from the Device Manager of interest to the AMS Client.
 *
 * @param[in]   p_ams            AMS Client structure.
 * @param[in]   p_bond_mgmr_evt  Event received from the Bond Manager.
 */
void ble_ams_c_on_device_manager_evt(ble_ams_c_t      * p_ams,
                                     dm_handle_t const * p_handle,
                                     dm_event_t const  * p_dm_evt);

/**@brief Function for initializing the AMS Client.
 *
 * @param[out]  p_ams        AMS Client structure. This structure will have to be
 *                           supplied by the application. It will be initialized by this function,
 *                           and will later be used to identify this particular client instance.
 * @param[in]   p_ams_init   Information needed to initialize the client.
 *
 * @return      NRF_SUCCESS on successful initialization of client, otherwise an error code.
 */
uint32_t ble_ams_c_init(ble_ams_c_t * p_ams, const ble_ams_c_init_t * p_ams_init);

uint32_t ble_ams_c_enable_notif_remote_control(const ble_ams_c_t * p_ams);

/**@brief Function for send remote command to AMS Client.
 *
 * @param[in]   p_ams        Apple Media structure. This structure will have to be supplied by
 *                           the application. It identifies the particular client instance to use.
 * @param[in]   p_cmd        Command to send through the client.
 *
 * @return      NRF_SUCCESS on successful initialization of client, otherwise an error code.
 */
uint32_t ble_ams_send_rc_command(ble_ams_c_t * p_ams, const ble_ams_remote_command_values_t p_cmd);

uint32_t ble_ams_c_service_load(const ble_ams_c_t * p_ams);

uint32_t ble_ams_c_service_store(void);

uint32_t ble_ams_c_service_delete(void);

#endif // BLE_AMS_H__
