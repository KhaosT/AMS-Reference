//
//  AMSTrackInfoWindow.swift
//  AMSDemo
//
//  Created by Khaos Tian on 7/11/14.
//  Copyright (c) 2014 Oltica. All rights reserved.
//

import Cocoa

class AMSTrackInfoWindow: NSWindowController, AMSMusicInfoDelegate {

    @IBOutlet var titleLabel: NSTextField
    @IBOutlet var albumLabel: NSTextField
    @IBOutlet var artistLabel: NSTextField
    @IBOutlet var durationLabel: NSTextField
    
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
    }
    
}
