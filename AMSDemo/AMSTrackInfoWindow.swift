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
    @IBOutlet var titleLabel: NSTextField?
    @IBOutlet var albumLabel: NSTextField?
    @IBOutlet var artistLabel: NSTextField?
    @IBOutlet var durationLabel: NSTextField?
    
    //Queue
    @IBOutlet var indexLabel: NSTextField?
    @IBOutlet var countLabel: NSTextField?
    @IBOutlet var shuffleModeLabel: NSTextField?
    @IBOutlet var repeatModeLabel: NSTextField?
    
    //Player
    @IBOutlet var nameLabel: NSTextField?
    @IBOutlet var playbackInfoLabel: NSTextField?
    @IBOutlet var volumeLabel: NSTextField?
    
    override func windowDidLoad() {
        super.windowDidLoad()
        
        // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    }
    
    func didUpdateMediaInfo(info:AMSTrackInfo!) {
        if info.title {
            if let titleLabel = titleLabel {
                titleLabel.stringValue = info.title
            }
        }
        if info.album {
            if let albumLabel = albumLabel {
                albumLabel.stringValue = info.album
            }
        }
        if info.artist {
            if let artistLabel = artistLabel {
                artistLabel.stringValue = info.artist
            }
        }
        if info.duration {
            if let durationLabel = durationLabel {
                durationLabel.stringValue = info.duration
            }
        }
        
        if info.index {
            if let indexLabel = indexLabel {
                indexLabel.stringValue = info.index
            }
        }
        if info.count {
            if let countLabel = countLabel {
                countLabel.stringValue = info.count
            }
        }
        if info.sfMode {
            if let shuffleModeLabel = shuffleModeLabel {
                shuffleModeLabel.stringValue = info.sfMode
            }
        }
        if info.rpMode {
            if let repeatModeLabel = repeatModeLabel {
                repeatModeLabel.stringValue = info.rpMode
            }
        }
        
        if info.name {
            if let nameLabel = nameLabel {
                nameLabel.stringValue = info.name
            }
        }
        if info.playbackInfo {
            if let playbackInfoLabel = playbackInfoLabel {
                playbackInfoLabel.stringValue = info.playbackInfo
            }
        }
        if info.volume {
            if let volumeLabel = volumeLabel {
                volumeLabel.stringValue = info.volume
            }
        }
    }
    
}
