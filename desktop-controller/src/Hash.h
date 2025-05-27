//
//  Hash.h
//  desktop-controller
//
//  Created by Dylan Marcus on 10/31/21.
//

#ifndef Hash_h
#define Hash_h

#include <stdint.h>
#include <atomic>
#include <vector>
#include <string>
#include <sstream>

class AntDevice {
public:
    std::vector<int> deviceValues;
    std::vector<int> deltaValues;
};

class HashObject: public ofThread{
public:
    
    HashObject(){
        lastHashTime = ofGetElapsedTimeMillis();
        hashUpdateInterval = 60000; // 1 minute in milliseconds
    }
    
    ~HashObject(){
        stop();
        waitForThread(false);
    }

    void stop(){
        stopThread();
    }
    
    // Generates a hash from combined sensor and video tracking data
    std::string generateHash(const std::vector<AntDevice>& deviceData, const std::vector<ofPoint>& trackedPoints) {
        std::string combinedData;
        
        // Add timestamp to make each hash unique even with same input
        combinedData += std::to_string(ofGetElapsedTimeMillis());
        
        // Add sensor data
        for (const auto& device : deviceData) {
            for (const auto& value : device.deviceValues) {
                combinedData += std::to_string(value);
            }
            for (const auto& delta : device.deltaValues) {
                combinedData += std::to_string(delta);
            }
        }
        
        // Add video tracking data
        for (const auto& point : trackedPoints) {
            combinedData += std::to_string(point.x);
            combinedData += std::to_string(point.y);
        }
        
        return murmurHash3(combinedData);
    }
    
    // Check if it's time to generate a new hash
    bool shouldUpdateHash() {
        uint64_t currentTime = ofGetElapsedTimeMillis();
        if (currentTime - lastHashTime >= hashUpdateInterval) {
            lastHashTime = currentTime;
            return true;
        }
        return false;
    }

private:
    uint64_t lastHashTime;
    uint64_t hashUpdateInterval;
    
    // MurmurHash3 implementation (better distribution than simple hash)
    std::string murmurHash3(const std::string& str, std::uint32_t seed = 0) {
        const std::uint32_t c1 = 0xcc9e2d51;
        const std::uint32_t c2 = 0x1b873593;
        
        std::uint32_t h1 = seed;
        
        // Body
        const std::uint32_t* blocks = (const std::uint32_t*)(str.c_str());
        for (int i = 0; i < str.length() / 4; i++) {
            std::uint32_t k1 = blocks[i];
            
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;
            
            h1 ^= k1;
            h1 = (h1 << 13) | (h1 >> 19);
            h1 = h1 * 5 + 0xe6546b64;
        }
        
        // Tail
        const std::uint8_t* tail = (const std::uint8_t*)(str.c_str() + (str.length() / 4) * 4);
        std::uint32_t k1 = 0;
        switch (str.length() & 3) {
            case 3:
                k1 ^= tail[2] << 16;
            case 2:
                k1 ^= tail[1] << 8;
            case 1:
                k1 ^= tail[0];
                k1 *= c1;
                k1 = (k1 << 15) | (k1 >> 17);
                k1 *= c2;
                h1 ^= k1;
        }
        
        // Finalization
        h1 ^= str.length();
        h1 ^= h1 >> 16;
        h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35;
        h1 ^= h1 >> 16;
        
        // Convert to hex string
        std::stringstream ss;
        ss << std::hex << h1;
        return ss.str();
    }

    /// Everything in this function will happen in a different
    /// thread which leaves the main thread completelty free for
    /// other task;s.
    void threadedFunction(){

    }
};

class UnHashObject: public ofThread{
public:
    
    UnHashObject(){

    }
    
    ~UnHashObject(){
        stop();
        waitForThread(false);
    }

    void stop(){
        stopThread();
    }

    /// Everything in this function will happen in a different
    /// thread which leaves the main thread completelty free for
    /// other task;s.
    void threadedFunction(){

    }
};

#endif /* Hash_h */
