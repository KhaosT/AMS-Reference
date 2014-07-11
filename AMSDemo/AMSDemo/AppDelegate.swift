//
//  AppDelegate.swift
//  AMSDemo
//
//  Created by Khaos Tian on 7/11/14.
//  Copyright (c) 2014 Oltica. All rights reserved.
//

import Cocoa
import CoreBluetooth

class AppDelegate: NSObject, NSApplicationDelegate, AMSDelegate, AMSMusicInfoDelegate, NSTableViewDelegate, NSTableViewDataSource {
                            
    @IBOutlet var window: NSWindow
    @IBOutlet var tableView: NSTableView
    @IBOutlet var connectButton: NSButton
    
    @IBOutlet var titleLabel: NSTextField
    @IBOutlet var albumLabel: NSTextField
    @IBOutlet var artistLabel: NSTextField
    @IBOutlet var durationLabel: NSTextField
    
    
    var mediaCore:AMSCore!
    
    var discoveredPeripherals = [CBPeripheral]()
    var peripheralsRSSI = [CBPeripheral:NSNumber]()
    
    var localAMSInstance:AMSInstance!

    func applicationDidFinishLaunching(aNotification: NSNotification?) {
        mediaCore = AMSCore(delegate: self)
        NSNotificationCenter.defaultCenter().addObserver(self, selector: "AMSDidFinishSetup:", name: "AMSDidFinishSetup", object: nil)
        // Insert code here to initialize your application
    }
    
    func AMSDidFinishSetup(aNotification:NSNotification!){
        if let aNotification = aNotification {
            localAMSInstance = aNotification.object as AMSInstance
            println("Ready to Control")
            localAMSInstance.subscribeUpdateForMusicInfo(self)
        }
    }
    
    func didUpdateMediaInfo(info:AMSTrackInfo!) {
        if info.title {
            titleLabel.stringValue = info.title
        }
        if info.album {
            albumLabel.stringValue = info.album
        }
        if info.artist {
            artistLabel.stringValue = info.artist
        }
        if info.duration {
            durationLabel.stringValue = info.duration
        }
    }

    func applicationWillTerminate(aNotification: NSNotification?) {
        // Insert code here to tear down your application
    }
    
    @IBAction func tableViewSelectionDidChange(sender: AnyObject) {
        connectButton.enabled = true
    }
    
    @IBAction func connectToDevice(sender: AnyObject) {
        if connectButton.title == "Connect" {
            tableView.enabled = false;
            mediaCore.connectToPeripheral(discoveredPeripherals[tableView.selectedRow])
            connectButton.title = "Disconnect"
        }else{
            tableView.enabled = true;
            mediaCore.disconnectPeripheral(discoveredPeripherals[tableView.selectedRow])
            connectButton.title = "Connect"
        }
    }
    
    @IBAction func sendRemoteCommand(sender: NSButton) {
        var command:AMSControlCommand
        switch sender.tag {
        case 0:
            command = AMSControlCommand.Play
        case 1:
            command = AMSControlCommand.Pause
        case 2:
            command = AMSControlCommand.TogglePlayPause
        case 3:
            command = AMSControlCommand.NextTrack
        case 4:
            command = AMSControlCommand.PreviousTrack
        case 5:
            command = AMSControlCommand.VolumeUp
        case 6:
            command = AMSControlCommand.VolumeDown
        case 7:
            command = AMSControlCommand.RepeatMode
        case 8:
            command = AMSControlCommand.ShuffleMode
        case 9:
            command = AMSControlCommand.SkipForward
        case 10:
            command = AMSControlCommand.SkipBackward
        default:
            println("Unknown command")
            return
        }
        localAMSInstance?.sendControlCommand(command)
    }
    
    func AMSCoreIsReadyToScan() {
        println("Ready To Scan")
        mediaCore.startScan()
    }
    
    func AMSCoreDidDiscoverPeripheral(peripheral:CBPeripheral!, advData:[NSObject : AnyObject]!, RSSI: NSNumber!) {
        if !contains(discoveredPeripherals, peripheral) {
            discoveredPeripherals += peripheral
        }
        peripheralsRSSI[peripheral] = RSSI
        dispatch_async(dispatch_get_main_queue()) {
            self.tableView.reloadData()
        }
    }
    
    func AMSCoreDidConnectPeripheral(peripheral:CBPeripheral!) {
        dispatch_async(dispatch_get_main_queue()){
            self.tableView.enabled = false;
            self.connectButton.title = "Disconnect"
        }
    }
    
    func AMSCoreDidDisconnectPeripheral(peripheral:CBPeripheral!) {
        localAMSInstance = nil
        dispatch_async(dispatch_get_main_queue()){
            self.tableView.enabled = true;
            self.connectButton.title = "Connect"
        }
    }
    
    func numberOfRowsInTableView(tableView: NSTableView!) -> Int {
        return discoveredPeripherals.count
    }
    
    func tableView(tableView: NSTableView!, objectValueForTableColumn tableColumn: NSTableColumn!, row: Int) -> AnyObject! {
        if tableColumn.identifier == "PeripheralColumnIdentifier" {
            if discoveredPeripherals[row].name {
                return discoveredPeripherals[row].name
            }else{
                return discoveredPeripherals[row].identifier.UUIDString
            }
        }
        
        if tableColumn.identifier == "RSSIColumnIdentifier" {
            return peripheralsRSSI[discoveredPeripherals[row]]
        }
        
        return "Hello"
    }

}

