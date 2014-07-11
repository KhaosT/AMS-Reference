import Foundation

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
    
    init() {
        
    }
    
    func updateInfoWithData(data:NSData!) {
        if let data = data {
            let rawData = RawData(rawData: data)
            if (rawData.read(0) as Byte == 0x02) {
                println("Type:Media Info")
                let type = AMSTrackDataType.fromRaw(rawData.read(1) as Byte) as AMSTrackDataType
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
                
            }else{
                println("Unknown Data:\(data)")
            }
        }
    }
}