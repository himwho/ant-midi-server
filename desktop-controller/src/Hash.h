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

class HashObject: public ofThread{
public:
    
    HashObject(){

    }
    
    ~HashObject(){
        stop();
        waitForThread(false);
    }

    void stop(){
        stopThread();
    }
 
    // replace all Math.imul with * but ensuring both are signed 32 int
    // replace seed with sepcific index of ant feed
    // replace bytes with ant feed
    void hash(std::string str, std::int32_t seed = 0) {
        std::int32_t h1 = 0xdeadbeef ^ seed;
        std::int32_t h2 = 0x41c6ce57 ^ seed;
        for (int i = 0, ch; i < str.length(); i++) {
            ch = (std::int32_t)str[i]; //char at index
            h1 = (h1 ^ ch) * 2654435761;
            h2 = (h2 ^ ch) * 1597334677;
        }
        h1 = ((h1 ^ (h1>>16)) * 2246822507) ^ ((h2 ^ (h2>>13)) * 3266489909);
        h2 = ((h2 ^ (h2>>16)) * 2246822507) ^ ((h1 ^ (h1>>13)) * 3266489909);
        return 4294967296 * (2097151 & h2) + (h1>>0);
    }
    
    void tinyHash(std::string s) {
        for(int i = 0; i < s.length();) {
            std::int32_t h = 9;
            h = (h^(std::int32_t)s[i++] * (9^9));
        return h^h>>9;
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
