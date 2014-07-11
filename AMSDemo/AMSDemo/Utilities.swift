import Foundation

class RawData {
    var data: NSData!
    
    init(rawData: NSData) {
        data = rawData
    }
    
    func read<T>(byteLocation: Int) -> T {
        let bytes = data.subdataWithRange(NSMakeRange(byteLocation, sizeof(T))).bytes
        return UnsafePointer<T>(bytes).memory
    }
}