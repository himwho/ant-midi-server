class SecurityHashGenerator {
    constructor() {
        this.sensorData = [];
        this.cvNotes = [];
        this.hashHistory = [];
        this.currentHash = '';
        this.lastHashUpdate = Date.now();
        this.totalHashesGenerated = 0;
        this.securityLevel = 'HIGH';
        
        // Initialize with some mock sensor data
        this.initializeSensors();
        
        // Start hash generation every second
        this.startHashGeneration();
    }
    
    initializeSensors() {
        const sensorNames = [
            'Habitat 001 - Brown Camponotus',
            'Habitat 002 - Clear Camponotus', 
            'Habitat 003 - Tan Camponotus',
            'Temperature Sensor',
            'Humidity Sensor',
            'Light Sensor',
            'Motion Detector',
            'Vibration Sensor'
        ];
        
        this.sensorData = sensorNames.map((name, index) => ({
            id: index,
            name: name,
            activity: Math.random() * 100,
            lastValue: Math.floor(Math.random() * 1024),
            timestamp: Date.now() - Math.random() * 60000
        }));
    }
    
    generateSecurityHash() {
        const timestamp = Date.now();
        
        // Collect entropy from various sources
        const sensorEntropy = this.collectSensorEntropy();
        const cvEntropy = this.collectCVEntropy();
        const timeEntropy = this.generateTimeEntropy();
        const randomEntropy = this.generateRandomEntropy();
        
        // Combine all entropy sources
        const combinedEntropy = sensorEntropy + cvEntropy + timeEntropy + randomEntropy;
        
        // Generate hash using crypto API
        return this.hashString(combinedEntropy + timestamp.toString());
    }
    
    collectSensorEntropy() {
        let entropy = '';
        this.sensorData.forEach(sensor => {
            entropy += sensor.activity.toString() + sensor.lastValue.toString();
        });
        return entropy;
    }
    
    collectCVEntropy() {
        let entropy = '';
        this.cvNotes.slice(0, 5).forEach(note => {
            entropy += note.content.length.toString() + note.timestamp.toString();
        });
        return entropy;
    }
    
    generateTimeEntropy() {
        const now = Date.now();
        return (now % 1000000).toString() + Math.sin(now / 1000).toString();
    }
    
    generateRandomEntropy() {
        const randomBytes = new Uint8Array(16);
        crypto.getRandomValues(randomBytes);
        return Array.from(randomBytes).map(b => b.toString(16).padStart(2, '0')).join('');
    }
    
    async hashString(input) {
        const encoder = new TextEncoder();
        const data = encoder.encode(input);
        const hashBuffer = await crypto.subtle.digest('SHA-256', data);
        const hashArray = Array.from(new Uint8Array(hashBuffer));
        return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
    }
    
    updateSensorData() {
        this.sensorData.forEach(sensor => {
            // Simulate sensor activity changes
            sensor.activity = Math.max(0, Math.min(100, 
                sensor.activity + (Math.random() - 0.5) * 20
            ));
            sensor.lastValue = Math.floor(Math.random() * 1024);
            sensor.timestamp = Date.now();
        });
    }
    
    addCVNote(content) {
        const note = {
            id: Date.now() + Math.random(),
            content: content,
            timestamp: Date.now()
        };
        this.cvNotes.unshift(note);
        
        // Keep only last 50 notes
        if (this.cvNotes.length > 50) {
            this.cvNotes = this.cvNotes.slice(0, 50);
        }
    }
    
    generateRandomCVNote() {
        const noteTypes = [
            'Motion detected in quadrant',
            'Temperature fluctuation observed',
            'Ant cluster formation detected',
            'Feeding activity increased',
            'Trail pheromone concentration spike',
            'Nest construction activity',
            'Foraging pattern change',
            'Colony communication burst'
        ];
        
        const quadrants = ['A1', 'A2', 'B1', 'B2', 'C1', 'C2'];
        const values = Math.floor(Math.random() * 100);
        
        const noteType = noteTypes[Math.floor(Math.random() * noteTypes.length)];
        const quadrant = quadrants[Math.floor(Math.random() * quadrants.length)];
        
        return `${noteType} ${quadrant}: ${values}%`;
    }
    
    startHashGeneration() {
        setInterval(async () => {
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
    }
    
    updateSecurityLevel() {
        const avgActivity = this.sensorData.reduce((sum, sensor) => sum + sensor.activity, 0) / this.sensorData.length;
        
        if (avgActivity > 70) {
            this.securityLevel = 'MAXIMUM';
        } else if (avgActivity > 50) {
            this.securityLevel = 'HIGH';
        } else if (avgActivity > 30) {
            this.securityLevel = 'MEDIUM';
        } else {
            this.securityLevel = 'LOW';
        }
    }
    
    getActiveSensorCount() {
        return this.sensorData.filter(sensor => sensor.activity > 10).length;
    }
}

// Initialize Vue app for security dashboard
const securityApp = new Vue({
    el: '#securityApp',
    data: {
        hashGenerator: new SecurityHashGenerator(),
        socket: null
    },
    computed: {
        currentSecurityHash() {
            return this.hashGenerator.currentHash || 'Initializing...';
        },
        lastHashUpdate() {
            return this.hashGenerator.lastHashUpdate;
        },
        totalHashesGenerated() {
            return this.hashGenerator.totalHashesGenerated;
        },
        activeSensors() {
            return this.hashGenerator.getActiveSensorCount();
        },
        cvNotesCount() {
            return this.hashGenerator.cvNotes.length;
        },
        securityLevel() {
            return this.hashGenerator.securityLevel;
        },
        sensorData() {
            return this.hashGenerator.sensorData;
        },
        cvNotes() {
            return this.hashGenerator.cvNotes;
        },
        hashHistory() {
            return this.hashGenerator.hashHistory;
        }
    },
    mounted() {
        this.initializeSocket();
        this.startRealtimeUpdates();
    },
    methods: {
        initializeSocket() {
            this.socket = io();
            
            // Listen for OSC data from ant sensors
            this.socket.on('osc', (data) => {
                this.handleSensorData(data);
            });
            
            // Listen for hash updates from the main system
            this.socket.on('hash', (hashData) => {
                this.handleHashUpdate(hashData);
            });
            
            // Listen for internal data
            this.socket.on('internal', (data) => {
                this.handleInternalData(data);
            });
        },
        
        handleSensorData(data) {
            // Update sensor data based on incoming OSC data
            const sensorIndex = data.x % this.hashGenerator.sensorData.length;
            if (this.hashGenerator.sensorData[sensorIndex]) {
                this.hashGenerator.sensorData[sensorIndex].activity = 
                    Math.max(0, Math.min(100, (data.y / 127) * 100));
                this.hashGenerator.sensorData[sensorIndex].lastValue = data.z;
                this.hashGenerator.sensorData[sensorIndex].timestamp = Date.now();
            }
            
            // Generate CV note based on sensor data
            if (data.y > 100) {
                const note = `High activity detected: Channel ${data.x}, Value ${data.y}, Velocity ${data.z}`;
                this.hashGenerator.addCVNote(note);
            }
        },
        
        handleHashUpdate(hashData) {
            // If we receive hash updates from the main system, use them
            this.hashGenerator.currentHash = hashData.value;
            this.hashGenerator.lastHashUpdate = hashData.timestamp;
            
            this.hashGenerator.hashHistory.unshift({
                value: hashData.value,
                timestamp: hashData.timestamp
            });
        },
        
        handleInternalData(data) {
            // Handle internal system data for additional entropy
            const note = `Internal system event: ${data.x}-${data.y}-${data.z}`;
            this.hashGenerator.addCVNote(note);
        },
        
        startRealtimeUpdates() {
            // Force Vue to update every second to show real-time changes
            setInterval(() => {
                this.$forceUpdate();
            }, 100);
        },
        
        exportHashHistory() {
            const dataStr = JSON.stringify(this.hashGenerator.hashHistory, null, 2);
            const dataBlob = new Blob([dataStr], {type: 'application/json'});
            const url = URL.createObjectURL(dataBlob);
            const link = document.createElement('a');
            link.href = url;
            link.download = `ant-security-hashes-${new Date().toISOString()}.json`;
            link.click();
            URL.revokeObjectURL(url);
        },
        
        clearHistory() {
            this.hashGenerator.hashHistory = [];
            this.hashGenerator.cvNotes = [];
        }
    }
});

// Add some utility functions for enhanced security features
window.SecurityUtils = {
    validateHash: function(hash) {
        return hash && hash.length === 64 && /^[a-f0-9]+$/i.test(hash);
    },
    
    calculateEntropy: function(data) {
        const freq = {};
        for (let char of data) {
            freq[char] = (freq[char] || 0) + 1;
        }
        
        let entropy = 0;
        const len = data.length;
        for (let char in freq) {
            const p = freq[char] / len;
            entropy -= p * Math.log2(p);
        }
        return entropy;
    },
    
    generateSecurityReport: function() {
        const app = securityApp;
        return {
            timestamp: new Date().toISOString(),
            currentHash: app.currentSecurityHash,
            totalHashes: app.totalHashesGenerated,
            activeSensors: app.activeSensors,
            securityLevel: app.securityLevel,
            avgSensorActivity: app.sensorData.reduce((sum, s) => sum + s.activity, 0) / app.sensorData.length,
            recentCVNotes: app.cvNotes.slice(0, 5)
        };
    }
};

console.log('🔐 Ant Colony Security Dashboard initialized');
console.log('Hash generation active - updating every second');
console.log('Monitoring', securityApp.sensorData.length, 'sensors for entropy generation'); 