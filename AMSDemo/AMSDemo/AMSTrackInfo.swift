import Foundation

enum AMSPlayerDataType : Byte {
    case Name = 0x00
    case PlaybackInfo
    case Volume
}

enum AMSQueueDataType : Byte {
    case Index = 0x00
    case Count
    case SfMode
    case RpMode
}

enum AMSTrackDataType : Byte {
    case Artist = 0x00
    case Album
    case Title
    case Duration
}

class AMSTrackInfo{
    var artist:String!
    var album:String!
    var title:String!
    var duration:String!
    
    var index:String!
    var count:String!
    var sfMode:String!
    var rpMode:String!
    
    var name:String!
    var playbackInfo:String!
    var volume:String!
    
    init() {
        
    }
    
    func updateInfoWithData(data:NSData!) {
        if let data = data {
            let rawData = RawData(rawData: data)
            switch rawData.read(0) as Byte {
            case 0x00:
                println("Type:Player Info")
                if let type = AMSPlayerDataType.fromRaw(rawData.read(1) as Byte) {
                    switch type {
                    case .Name:
                        name = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Name:\(name)")
                        
                    case .PlaybackInfo:
                        playbackInfo = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("PlaybackInfo:\(playbackInfo)")
                        
                    case .Volume:
                        volume = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Volume:\(volume)")
                        
                    default:
                        println("Unknown")
                    }
                }
                
            case 0x01:
                println("Type:Queue Info")
                if let type = AMSQueueDataType.fromRaw(rawData.read(1) as Byte) {
                    switch type {
                    case .Index:
                        index = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Index:\(index)")
                        
                    case .Count:
                        count = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Count:\(count)")
                        
                    case .SfMode:
                        sfMode = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Shuffle Mode:\(sfMode)")
                        
                    case .RpMode:
                        rpMode = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Repeat Mode:\(rpMode)")
                        
                    default:
                        println("Unknown")
                    }
                }
                
            case 0x02:
                println("Type:Track Info")
                if let type = AMSTrackDataType.fromRaw(rawData.read(1) as Byte) {
                    switch type {
                    case .Artist:
                        artist = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Artist:\(artist)")
                        
                    case .Album:
                        album = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Album:\(album)")
                        
                    case .Title:
                        title = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Title:\(title)")
                        
                    case .Duration:
                        duration = NSString(data: data.subdataWithRange(NSMakeRange(3, data.length-3)), encoding: NSUTF8StringEncoding)
                        println("Duration:\(duration)")
                    default:
                        println("Unknown")
                    }
                }

            default:
                println("Unknown Data:\(data)")
            }
        }
    }
}