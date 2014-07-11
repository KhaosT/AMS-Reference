import Foundation
import CoreBluetooth

enum AMSControlCommand:Byte {
    case Play = 0x00
    case Pause
    case TogglePlayPause
    case NextTrack
    case PreviousTrack
    case VolumeUp
    case VolumeDown
    case RepeatMode
    case ShuffleMode
    case SkipForward
    case SkipBackward
}

protocol AMSMusicInfoDelegate {
    func didUpdateMediaInfo(info:AMSTrackInfo!)
}

class AMSInstance : NSObject,CBPeripheralDelegate{
    var internalPeripheral:CBPeripheral!
    var remoteCommandCharacteristic:CBCharacteristic!
    var entityUpdateCharacteristic:CBCharacteristic!
    var entityAttrCharacteristic:CBCharacteristic!
    
    var trackInfo:AMSTrackInfo!
    var musicDelegate:AMSMusicInfoDelegate!
    
    init(peripheral:CBPeripheral!){
        super.init()
        self.internalPeripheral = peripheral
        self.internalPeripheral.delegate = self
        self.internalPeripheral.discoverServices(nil)
    }
    
    func sendControlCommand(command:AMSControlCommand!){
        internalPeripheral.writeValue(NSData(bytes: [command.toRaw()] as [Byte], length: 1), forCharacteristic: remoteCommandCharacteristic, type: CBCharacteristicWriteType.WithResponse)
    }
    
    func peripheral(peripheral: CBPeripheral!, didDiscoverServices error: NSError!) {
        println("didDiscoverServices")
        for service in peripheral.services as [CBService] {
            if service.UUID == CBUUID.UUIDWithString("89D3502B-0F36-433A-8EF4-C502AD55F8DC") {
                println("Find Service")
                peripheral.discoverCharacteristics(nil, forService: service)
            }
        }
    }
    
    func subscribeUpdateForMusicInfo(delegate:AMSMusicInfoDelegate) {
        self.musicDelegate = delegate
        internalPeripheral.setNotifyValue(true, forCharacteristic: entityUpdateCharacteristic)
        trackInfo = AMSTrackInfo()
    }
    
    func peripheral(peripheral: CBPeripheral!, didDiscoverCharacteristicsForService service: CBService!, error: NSError!) {
        println("didDiscoverCharacteristicsForService:\(service)")
        for characteristic in service.characteristics as [CBCharacteristic]{
            if characteristic.UUID == CBUUID.UUIDWithString("9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2"){
                println("Find AMS Remote Command")
                remoteCommandCharacteristic = characteristic
                //Pairing and Notify
                peripheral.setNotifyValue(true, forCharacteristic: remoteCommandCharacteristic)
            }
            if characteristic.UUID == CBUUID.UUIDWithString("2F7CABCE-808D-411F-9A0C-BB92BA96C102"){
                println("Find AMS Entity Update")
                entityUpdateCharacteristic = characteristic
            }
            if characteristic.UUID == CBUUID.UUIDWithString("C6B2F38C-23AB-46D8-A6AB-A3A870BBD5D7"){
                println("AMS Entity Attribute")
                entityAttrCharacteristic = characteristic
            }
        }
        if remoteCommandCharacteristic != nil && entityAttrCharacteristic != nil && entityUpdateCharacteristic != nil {
            println("Finish Setup AMS")
            NSNotificationCenter.defaultCenter().postNotificationName("AMSDidFinishSetup", object: self)
        }
    }
    
    func peripheral(peripheral: CBPeripheral!, didUpdateValueForCharacteristic characteristic: CBCharacteristic!, error: NSError!) {
        if characteristic == entityUpdateCharacteristic {
            trackInfo.updateInfoWithData(characteristic.value())
            self.musicDelegate.didUpdateMediaInfo(trackInfo)
        }
    }
    
    func peripheral(peripheral: CBPeripheral!, didUpdateNotificationStateForCharacteristic characteristic: CBCharacteristic!, error: NSError!) {
        println("didUpdateNotificationStateForCharacteristic:\(characteristic),error:\(error)")
        if characteristic == entityUpdateCharacteristic {
            //Artist,Album,Title,Duration
            internalPeripheral.writeValue(NSData(bytes: [0x02,0x00,0x01,0x02,0x03] as [Byte], length: 5), forCharacteristic: entityUpdateCharacteristic, type: CBCharacteristicWriteType.WithResponse)
        }
    }
}