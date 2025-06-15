class AntCrypt {
    constructor() {
        this.currentHash = null;
        this.hashHistory = [];
        this.maxHashHistory = 1000; // Store last ~16 hours of hashes
        
        // Logging system
        this.loggingEnabled = false;
        this.sensorLoggingEnabled = false;
        this.logBuffer = [];
        this.maxLogBuffer = 10000; // Maximum log entries to keep in memory
        this.logInterval = null;
        this.autoSaveInterval = 300000; // Auto-save every 5 minutes when logging
    }

    // Store new hash with timestamp
    updateHash(hash, sensorData = null) {
        const hashEntry = {
            hash: hash,
            timestamp: Date.now(),
            sensorData: sensorData
        };
        this.currentHash = hashEntry;
        this.hashHistory.unshift(hashEntry);
        
        // Log if enabled
        if (this.loggingEnabled) {
            this.addToLog('HASH_UPDATE', {
                hash: hash,
                timestamp: hashEntry.timestamp,
                sensorData: this.sensorLoggingEnabled ? sensorData : null
            });
        }
        
        // Maintain history limit
        if (this.hashHistory.length > this.maxHashHistory) {
            this.hashHistory.pop();
        }
    }

    // Add entry to log buffer
    addToLog(type, data) {
        const logEntry = {
            timestamp: Date.now(),
            type: type,
            data: data,
            id: this.generateLogId()
        };
        
        this.logBuffer.unshift(logEntry);
        
        // Maintain log buffer limit
        if (this.logBuffer.length > this.maxLogBuffer) {
            this.logBuffer.pop();
        }
        
        console.log(`🔐 [ENCRYPTION-LOG] ${type}:`, data);
    }

    // Generate unique log ID
    generateLogId() {
        return Date.now().toString(36) + Math.random().toString(36).substr(2);
    }

    // Enable hash logging only
    enableHashLogging() {
        this.loggingEnabled = true;
        this.sensorLoggingEnabled = false;
        this.startAutoSave();
        this.addToLog('LOGGING_ENABLED', { mode: 'HASH_ONLY' });
        console.log('🔐 Hash logging enabled - Only hash keys will be logged');
        return 'Hash logging enabled';
    }

    // Enable full logging (hash + sensor data)
    enableFullLogging() {
        this.loggingEnabled = true;
        this.sensorLoggingEnabled = true;
        this.startAutoSave();
        this.addToLog('LOGGING_ENABLED', { mode: 'HASH_AND_SENSOR' });
        console.log('🔐 Full logging enabled - Hash keys and sensor data will be logged');
        return 'Full logging enabled (hash + sensor data)';
    }

    // Disable logging
    disableLogging() {
        this.loggingEnabled = false;
        this.sensorLoggingEnabled = false;
        this.stopAutoSave();
        this.addToLog('LOGGING_DISABLED', {});
        console.log('🔐 Logging disabled');
        return 'Logging disabled';
    }

    // Start auto-save interval
    startAutoSave() {
        if (this.logInterval) {
            clearInterval(this.logInterval);
        }
        this.logInterval = setInterval(() => {
            if (this.logBuffer.length > 0) {
                this.autoSaveLog();
            }
        }, this.autoSaveInterval);
    }

    // Stop auto-save interval
    stopAutoSave() {
        if (this.logInterval) {
            clearInterval(this.logInterval);
            this.logInterval = null;
        }
    }

    // Auto-save log to downloads
    autoSaveLog() {
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = `encryption-log-${timestamp}.json`;
        this.downloadLog(filename);
        console.log(`🔐 Auto-saved log to ${filename}`);
    }

    // Download current log buffer
    downloadLog(customFilename = null) {
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
        const filename = customFilename || `encryption-log-${timestamp}.json`;
        
        const logData = {
            exportTimestamp: Date.now(),
            exportDate: new Date().toISOString(),
            loggingMode: {
                hashLogging: this.loggingEnabled,
                sensorLogging: this.sensorLoggingEnabled
            },
            totalEntries: this.logBuffer.length,
            logEntries: this.logBuffer,
            hashHistory: this.hashHistory.slice(0, 100), // Include recent hash history
            metadata: {
                version: '1.0',
                source: 'AntCrypt Encryption Logger',
                maxBufferSize: this.maxLogBuffer
            }
        };
        
        const dataStr = JSON.stringify(logData, null, 2);
        const dataBlob = new Blob([dataStr], { type: 'application/json' });
        
        const url = URL.createObjectURL(dataBlob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);
        
        console.log(`🔐 Downloaded log file: ${filename} (${this.logBuffer.length} entries)`);
        return `Downloaded: ${filename}`;
    }

    // Clear log buffer
    clearLogBuffer() {
        const clearedCount = this.logBuffer.length;
        this.logBuffer = [];
        this.addToLog('BUFFER_CLEARED', { clearedEntries: clearedCount });
        console.log(`🔐 Cleared ${clearedCount} log entries`);
        return `Cleared ${clearedCount} log entries`;
    }

    // Get logging status
    getLoggingStatus() {
        return {
            hashLogging: this.loggingEnabled,
            sensorLogging: this.sensorLoggingEnabled,
            bufferSize: this.logBuffer.length,
            maxBufferSize: this.maxLogBuffer,
            autoSaveEnabled: this.logInterval !== null,
            hashHistorySize: this.hashHistory.length
        };
    }

    // Log sensor data manually (can be called from external sources)
    logSensorUpdate(sensorData) {
        if (this.sensorLoggingEnabled) {
            this.addToLog('SENSOR_UPDATE', sensorData);
        }
    }

    // Log encryption/decryption operations
    logCryptoOperation(operation, details) {
        if (this.loggingEnabled) {
            this.addToLog(`CRYPTO_${operation.toUpperCase()}`, details);
        }
    }

    // Convert hash to encryption key
    async hashToKey(hash) {
        const encoder = new TextEncoder();
        const data = encoder.encode(hash);
        const hashBuffer = await crypto.subtle.digest('SHA-256', data);
        return hashBuffer;
    }

    // Encrypt file using current hash
    async encryptFile(file) {
        if (!this.currentHash) {
            throw new Error('No hash available for encryption');
        }

        const key = await this.hashToKey(this.currentHash.hash);
        const iv = crypto.getRandomValues(new Uint8Array(12));
        const cryptoKey = await crypto.subtle.importKey(
            'raw',
            key,
            { name: 'AES-GCM' },
            false,
            ['encrypt']
        );

        const fileData = await file.arrayBuffer();
        const encryptedContent = await crypto.subtle.encrypt(
            {
                name: 'AES-GCM',
                iv: iv
            },
            cryptoKey,
            fileData
        );

        // Combine IV and encrypted content
        const encryptedFile = new Blob([
            iv.buffer,
            encryptedContent
        ], { type: 'application/octet-stream' });

        const result = {
            file: encryptedFile,
            timestamp: this.currentHash.timestamp,
            hash: this.currentHash.hash
        };

        // Log encryption operation
        this.logCryptoOperation('ENCRYPT', {
            fileName: file.name,
            fileSize: file.size,
            timestamp: this.currentHash.timestamp,
            hash: this.currentHash.hash,
            encryptedSize: encryptedFile.size
        });

        return result;
    }

    // Decrypt file using hash from specific timestamp
    async decryptFile(encryptedFile, timestamp) {
        // Find hash closest to timestamp
        const hashEntry = this.findHashByTimestamp(timestamp);
        if (!hashEntry) {
            throw new Error('Encryption hash not found for this timestamp');
        }

        const key = await this.hashToKey(hashEntry.hash);
        const fileData = await encryptedFile.arrayBuffer();
        
        // Extract IV and encrypted content
        const iv = new Uint8Array(fileData.slice(0, 12));
        const encryptedContent = fileData.slice(12);

        const cryptoKey = await crypto.subtle.importKey(
            'raw',
            key,
            { name: 'AES-GCM' },
            false,
            ['decrypt']
        );

        const decryptedContent = await crypto.subtle.decrypt(
            {
                name: 'AES-GCM',
                iv: iv
            },
            cryptoKey,
            encryptedContent
        );

        const result = new Blob([decryptedContent]);

        // Log decryption operation
        this.logCryptoOperation('DECRYPT', {
            timestamp: timestamp,
            hash: hashEntry.hash,
            encryptedSize: encryptedFile.size,
            decryptedSize: result.size
        });

        return result;
    }

    findHashByTimestamp(timestamp) {
        return this.hashHistory.find(entry => 
            Math.abs(entry.timestamp - timestamp) < 60000
        );
    }
}

// Initialize global AntCrypt instance
window.antCrypt = window.antCrypt || new AntCrypt();

// Global console functions for encryption logging
window.enableHashLogging = function() {
    return window.antCrypt.enableHashLogging();
};

window.enableFullLogging = function() {
    return window.antCrypt.enableFullLogging();
};

window.disableEncryptionLogging = function() {
    return window.antCrypt.disableLogging();
};

window.downloadEncryptionLog = function(filename = null) {
    return window.antCrypt.downloadLog(filename);
};

window.clearEncryptionLog = function() {
    return window.antCrypt.clearLogBuffer();
};

window.getEncryptionStatus = function() {
    const status = window.antCrypt.getLoggingStatus();
    console.table(status);
    return status;
};

// Manual hash update for testing
window.testHashUpdate = function(customHash = null, customSensorData = null) {
    const hash = customHash || 'test-hash-' + Date.now().toString(36);
    const sensorData = customSensorData || {
        testSensor1: Math.random() * 100,
        testSensor2: Math.random() * 100,
        timestamp: Date.now()
    };
    
    window.antCrypt.updateHash(hash, sensorData);
    console.log('🔐 Test hash update:', hash);
    return `Test hash logged: ${hash}`;
};

// Delayed initialization and integration with security dashboard
function initializeSecurityIntegration() {
    // Try to hook into security dashboard
    if (typeof window.securityApp !== 'undefined' && window.securityApp.hashGenerator) {
        const hashGen = window.securityApp.hashGenerator;
        
        // Hook into the actual hash generation method
        const originalGenerateSecurityHash = hashGen.generateSecurityHash;
        if (originalGenerateSecurityHash) {
            hashGen.generateSecurityHash = async function() {
                const hash = await originalGenerateSecurityHash.call(this);
                
                // Get sensor data if available
                const sensorData = this.sensorData || null;
                
                // Update AntCrypt with new hash and sensor data
                window.antCrypt.updateHash(hash, sensorData);
                
                return hash;
            };
            console.log('🔐 Hooked into security dashboard hash generation');
        }
        
        // Also hook into the hash update cycle in startHashGeneration
        const originalStartHashGeneration = hashGen.startHashGeneration;
        if (originalStartHashGeneration && !hashGen._encryptionLoggerHooked) {
            hashGen._encryptionLoggerHooked = true;
            
            // Override the interval to capture hash updates
            const originalSetInterval = setInterval;
            hashGen.startHashGeneration = function() {
                originalSetInterval(async () => {
                    // Update sensor data
                    this.updateSensorData();
                    
                    // Occasionally add CV notes
                    if (Math.random() < 0.3) {
                        this.addCVNote(this.generateRandomCVNote());
                    }
                    
                    // Generate new hash
                    this.currentHash = await this.generateSecurityHash();
                    this.lastHashUpdate = Date.now();
                    this.totalHashesGenerated++;
                    
                    // LOG THE HASH UPDATE HERE
                    window.antCrypt.updateHash(this.currentHash, this.sensorData);
                    
                    // Add to history
                    this.hashHistory.unshift({
                        value: this.currentHash,
                        timestamp: this.lastHashUpdate
                    });
                    
                    // Keep only last 100 hashes
                    if (this.hashHistory.length > 100) {
                        this.hashHistory = this.hashHistory.slice(0, 100);
                    }
                    
                    // Update security level based on activity
                    this.updateSecurityLevel();
                    
                }, 1000); // Update every second
            };
            console.log('🔐 Hooked into security dashboard hash generation cycle');
        }
        
        return true;
    }
    return false;
}

// Try immediate integration
let integrationAttempts = 0;
function attemptIntegration() {
    if (initializeSecurityIntegration()) {
        console.log('🔐 Security dashboard integration successful');
        return;
    }
    
    integrationAttempts++;
    if (integrationAttempts < 10) {
        // Retry every 500ms for up to 5 seconds
        setTimeout(attemptIntegration, 500);
    } else {
        console.log('🔐 Security dashboard not found - manual hash updates required');
    }
}

// Start integration attempts
attemptIntegration();

// Auto-connect to sensor data if available
if (typeof io !== 'undefined') {
    const socket = io();
    socket.on('osc', (data) => {
        if (window.antCrypt.sensorLoggingEnabled) {
            window.antCrypt.logSensorUpdate({
                type: 'OSC_DATA',
                channel: data.x,
                value: data.y,
                velocity: data.z,
                timestamp: Date.now()
            });
        }
    });
    
    socket.on('hash', (hashData) => {
        // Update with received hash data
        window.antCrypt.updateHash(hashData.hash || hashData, null);
    });
}

// Console startup message
console.log(`
🔐 AntCrypt Encryption Logger Loaded
Available Commands:
- enableHashLogging()        : Enable hash key logging only
- enableFullLogging()        : Enable hash + sensor data logging  
- disableEncryptionLogging() : Disable all encryption logging
- downloadEncryptionLog()    : Download current log as JSON file
- clearEncryptionLog()       : Clear current log buffer
- getEncryptionStatus()      : Show current logging status
- testHashUpdate()           : Generate a test hash update for debugging

Features:
✅ Client-side hash key logging
✅ Optional sensor data inclusion
✅ Auto-save every 5 minutes when active
✅ Downloadable JSON log files
✅ 10,000 entry buffer capacity
✅ Crypto operation tracking (encrypt/decrypt)
✅ Automatic security dashboard integration

Perfect for debugging and security analysis!

Testing: If you're not seeing hash updates, try testHashUpdate() to verify logging works.
`);