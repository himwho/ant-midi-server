class AntCrypt {
    constructor() {
        this.currentHash = null;
        this.hashHistory = [];
        this.maxHashHistory = 1000; // Store last ~16 hours of hashes
    }

    // Store new hash with timestamp
    updateHash(hash) {
        const hashEntry = {
            hash: hash,
            timestamp: Date.now()
        };
        this.currentHash = hashEntry;
        this.hashHistory.unshift(hashEntry);
        
        // Maintain history limit
        if (this.hashHistory.length > this.maxHashHistory) {
            this.hashHistory.pop();
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

        return {
            file: encryptedFile,
            timestamp: this.currentHash.timestamp,
            hash: this.currentHash.hash
        };
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

        return new Blob([decryptedContent]);
    }

    findHashByTimestamp(timestamp) {
        return this.hashHistory.find(entry => 
            Math.abs(entry.timestamp - timestamp) < 60000
        );
    }
}