// Test script for Ant Colony Encryption
const fs = require('fs');
const crypto = require('crypto');

// Simulate the AntCrypt class functionality
class TestAntCrypt {
    constructor() {
        this.currentHash = null;
        this.hashHistory = [];
    }

    updateHash(hash) {
        const hashEntry = {
            hash: hash,
            timestamp: Date.now()
        };
        this.currentHash = hashEntry;
        this.hashHistory.unshift(hashEntry);
    }

    async hashToKey(hash) {
        const data = Buffer.from(hash, 'utf8');
        const hashBuffer = crypto.createHash('sha256').update(data).digest();
        return hashBuffer;
    }

    async encryptFile(fileBuffer) {
        if (!this.currentHash) {
            throw new Error('No hash available for encryption');
        }

        const key = await this.hashToKey(this.currentHash.hash);
        const iv = crypto.randomBytes(12);
        
        const cipher = crypto.createCipher('aes-256-cbc', key);
        
        let encrypted = cipher.update(fileBuffer);
        encrypted = Buffer.concat([encrypted, cipher.final()]);

        // Combine IV and encrypted content
        const encryptedFile = Buffer.concat([iv, encrypted]);

        return {
            file: encryptedFile,
            timestamp: this.currentHash.timestamp
        };
    }

    async decryptFile(encryptedBuffer, timestamp) {
        const hashEntry = this.hashHistory.find(entry => 
            Math.abs(entry.timestamp - timestamp) < 60000
        );
        
        if (!hashEntry) {
            throw new Error('Encryption hash not found for this timestamp');
        }

        const key = await this.hashToKey(hashEntry.hash);
        
        // Extract IV and encrypted content
        const iv = encryptedBuffer.slice(0, 12);
        const encryptedContent = encryptedBuffer.slice(12);

        const decipher = crypto.createDecipher('aes-256-cbc', key);

        let decrypted = decipher.update(encryptedContent);
        decrypted = Buffer.concat([decrypted, decipher.final()]);

        return decrypted;
    }
}

async function testEncryption() {
    console.log('🔐 Testing Ant Colony Encryption...\n');

    // Create test instance
    const antCrypt = new TestAntCrypt();
    
    // Generate a test hash (simulating ant colony activity)
    const testHash = crypto.randomBytes(32).toString('hex');
    antCrypt.updateHash(testHash);
    
    console.log('✅ Generated test hash:', testHash.substring(0, 16) + '...');
    console.log('⏰ Timestamp:', antCrypt.currentHash.timestamp);

    try {
        // Read test file
        const testFile = fs.readFileSync('test.txt');
        console.log('📁 Original file size:', testFile.length, 'bytes');
        console.log('📄 Original content preview:', testFile.toString().substring(0, 50) + '...\n');

        // Encrypt file
        console.log('🔒 Encrypting file...');
        const encryptResult = await antCrypt.encryptFile(testFile);
        console.log('✅ Encryption successful!');
        console.log('📦 Encrypted file size:', encryptResult.file.length, 'bytes');
        
        // Save encrypted file
        fs.writeFileSync('test_encrypted.ant', encryptResult.file);
        console.log('💾 Encrypted file saved as test_encrypted.ant\n');

        // Decrypt file
        console.log('🔓 Decrypting file...');
        const decryptedBuffer = await antCrypt.decryptFile(encryptResult.file, encryptResult.timestamp);
        console.log('✅ Decryption successful!');
        console.log('📄 Decrypted content preview:', decryptedBuffer.toString().substring(0, 50) + '...');
        
        // Verify content matches
        if (testFile.equals(decryptedBuffer)) {
            console.log('🎉 SUCCESS: Original and decrypted files match perfectly!');
        } else {
            console.log('❌ ERROR: Files do not match!');
        }

        // Save decrypted file for verification
        fs.writeFileSync('test_decrypted.txt', decryptedBuffer);
        console.log('💾 Decrypted file saved as test_decrypted.txt');

        // Test with wrong timestamp (should fail)
        console.log('\n🧪 Testing with wrong timestamp...');
        try {
            await antCrypt.decryptFile(encryptResult.file, Date.now() + 100000);
            console.log('❌ ERROR: Decryption should have failed with wrong timestamp!');
        } catch (error) {
            console.log('✅ Expected error with wrong timestamp:', error.message);
        }

        console.log('\n🎯 Encryption test completed successfully!');
        
        // Display encryption record info
        console.log('\n📋 Encryption Record:');
        console.log('Hash:', testHash);
        console.log('Timestamp:', encryptResult.timestamp);
        console.log('Date:', new Date(encryptResult.timestamp).toLocaleString());

    } catch (error) {
        console.error('❌ Test failed:', error.message);
        console.error(error.stack);
    }
}

// Run the test
testEncryption().catch(console.error); 