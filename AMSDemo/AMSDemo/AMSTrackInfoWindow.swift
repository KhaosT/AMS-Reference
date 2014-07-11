//
//  AMSTrackInfoWindow.swift
//  AMSDemo
//
//  Created by Khaos Tian on 7/11/14.
//  Copyright (c) 2014 Oltica. All rights reserved.
//

import Cocoa

class AMSTrackInfoWindow: NSWindowController, AMSMusicInfoDelegate {

    //Track
    @IBOutlet var titleLabel: NSTextField
    @IBOutlet var albumLabel: NSTextField
    @IBOutlet var artistLabel: NSTextField
    @IBOutlet var durationLabel: NSTextField
    
    //Queue
    @IBOutlet var indexLabel: NSTextField
    @IBOutlet var countLabel: NSTextField
    @IBOutlet var shuffleModeLabel: NSTextField
    @IBOutlet var repeatModeLabel: NSTextField
    
    //Player
    @IBOutlet var nameLabel: NSTextField
    @IBOutlet var playbackInfoLabel: NSTextField
    @IBOutlet var volumeLabel: NSTextField
    
    
    override func windowDidLoad() {
        super.windowDidLoad()

        // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
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
        
        if info.index {
            indexLabel.stringValue = info.index
        }
        if info.count {
            countLabel.stringValue = info.count
        }
        if info.sfMode {
            shuffleModeLabel.stringValue = info.sfMode
        }
        if info.rpMode {
            repeatModeLabel.stringValue = info.rpMode
        }
        
        if info.name {
            nameLabel.stringValue = info.name
        }
        if info.playbackInfo {
            playbackInfoLabel.stringValue = info.playbackInfo
        }
        if info.volume {
            volumeLabel.stringValue = info.volume
        }
    }
    
}
