//
//  AMSCore.swift
//  AMSDemo
//
//  Created by Khaos Tian on 7/11/14.
//  Copyright (c) 2014 Oltica. All rights reserved.
//

import Cocoa
import CoreBluetooth

protocol AMSDelegate {
    func AMSCoreIsReadyToScan()
    func AMSCoreDidDiscoverPeripheral(peripheral:CBPeripheral!, advData:[NSObject : AnyObject]!, RSSI: NSNumber!)
    func AMSCoreDidConnectPeripheral(peripheral:CBPeripheral!)
    func AMSCoreDidDisconnectPeripheral(peripheral:CBPeripheral!)
}

class AMSCore: NSObject, CBCentralManagerDelegate {
    
    var centralManager:CBCentralManager!
    var operationQueue = dispatch_queue_create("org.oltica.centralQueue", DISPATCH_QUEUE_SERIAL)
    
    var _delegate:AMSDelegate!
    var _amsInstance:AMSInstance!
        
    init(delegate:AMSDelegate!) {
        super.init()
        self._delegate = delegate
        centralManager = CBCentralManager(delegate: self, queue: operationQueue)
    }
    
    func startScan() {
        centralManager.scanForPeripheralsWithServices(nil, options: [CBCentralManagerScanOptionAllowDuplicatesKey:true])
    }
    
    func connectToPeripheral(peripheral:CBPeripheral!) {
        centralManager.connectPeripheral(peripheral, options: nil)
    }
    
    func disconnectPeripheral(peripheral:CBPeripheral!) {
        centralManager.cancelPeripheralConnection(peripheral)
    }
    
    func centralManager(central: CBCentralManager!, didConnectPeripheral peripheral: CBPeripheral!) {
        println("didConnectPeripheral")
        _amsInstance = AMSInstance(peripheral: peripheral)
        _delegate?.AMSCoreDidConnectPeripheral(peripheral)
    }
    
    func centralManager(central: CBCentralManager!, didDisconnectPeripheral peripheral: CBPeripheral!, error: NSError!) {
        println("didDisconnectPeripheral")
        _amsInstance?.didDisconnectFromPeripheral()
        _amsInstance = nil
        _delegate?.AMSCoreDidDisconnectPeripheral(peripheral)
    }
    
    func centralManagerDidUpdateState(central: CBCentralManager!) {
        switch central.state {
        case CBCentralManagerState.PoweredOn:
            println("Central is ready")
            self._delegate?.AMSCoreIsReadyToScan()
        default:
            println("Unexpected State")
        }
    }
    
    func centralManager(central: CBCentralManager!, didDiscoverPeripheral peripheral: CBPeripheral!, advertisementData: [NSObject : AnyObject]!, RSSI: NSNumber!) {
        self._delegate?.AMSCoreDidDiscoverPeripheral(peripheral, advData: advertisementData, RSSI:RSSI)
    }
}
