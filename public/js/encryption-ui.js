class EncryptionUI {
    constructor() {
        this.antCrypt = new AntCrypt();
        this.encryptionRecords = JSON.parse(localStorage.getItem('antEncryptionRecords') || '[]');
        this.initializeUI();
        this.updateRecordsDisplay();
    }

    initializeUI() {
        // File input handlers
        const fileToEncrypt = document.getElementById('fileToEncrypt');
        const fileToDecrypt = document.getElementById('fileToDecrypt');
        const encryptButton = document.getElementById('encryptButton');
        const decryptButton = document.getElementById('decryptButton');
        const clearRecordsButton = document.getElementById('clearRecordsButton');

        // File selection handlers
        fileToEncrypt?.addEventListener('change', (e) => {
            const fileName = e.target.files[0]?.name || 'No file selected';
            document.getElementById('encryptFileName').textContent = fileName;
            encryptButton.disabled = !e.target.files[0];
        });

        fileToDecrypt?.addEventListener('change', (e) => {
            const fileName = e.target.files[0]?.name || 'No file selected';
            document.getElementById('decryptFileName').textContent = fileName;
            decryptButton.disabled = !e.target.files[0];
        });

        // Encryption button handler
        encryptButton?.addEventListener('click', () => this.handleEncryption());
        
        // Decryption button handler
        decryptButton?.addEventListener('click', () => this.handleDecryption());
        
        // Clear records button handler
        clearRecordsButton?.addEventListener('click', () => this.clearRecords());

        // Update hash from security dashboard
        this.startHashSync();
    }

    startHashSync() {
        setInterval(() => {
            if (window.securityApp && window.securityApp.currentSecurityHash) {
                this.antCrypt.updateHash(window.securityApp.currentSecurityHash);
            }
        }, 1000);
    }

    async handleEncryption() {
        const fileInput = document.getElementById('fileToEncrypt');
        const file = fileInput.files[0];
        
        if (!file) {
            this.showNotification('Please select a file to encrypt', 'error');
            return;
        }

        if (!this.antCrypt.currentHash) {
            this.showNotification('No encryption hash available. Please wait...', 'error');
            return;
        }

        try {
            this.showNotification('Encrypting file...', 'info');
            
            const result = await this.antCrypt.encryptFile(file);
            
            // Create download link with timestamp in filename
            const url = URL.createObjectURL(result.file);
            const link = document.createElement('a');
            link.href = url;
            link.download = `encrypted_${result.timestamp}_${file.name}.ant`;
            link.click();
            URL.revokeObjectURL(url);

            // Store encryption record
            const record = {
                id: Date.now(),
                originalFileName: file.name,
                encryptedFileName: `encrypted_${result.timestamp}_${file.name}.ant`,
                hash: result.hash,
                timestamp: result.timestamp,
                dateCreated: new Date().toLocaleString(),
                fileSize: file.size
            };

            this.encryptionRecords.unshift(record);
            this.saveRecords();
            this.updateRecordsDisplay();

            this.showNotification(`File encrypted successfully! Hash: ${record.hash.substring(0, 16)}...`, 'success');
            
            // Reset file input
            fileInput.value = '';
            document.getElementById('encryptFileName').textContent = 'No file selected';
            document.getElementById('encryptButton').disabled = true;

        } catch (error) {
            console.error('Encryption error:', error);
            this.showNotification(`Encryption failed: ${error.message}`, 'error');
        }
    }

    async handleDecryption() {
        const fileInput = document.getElementById('fileToDecrypt');
        const file = fileInput.files[0];
        
        if (!file) {
            this.showNotification('Please select a file to decrypt', 'error');
            return;
        }

        // Try to extract timestamp from filename or ask user
        let timestamp = this.extractTimestampFromFilename(file.name);
        
        if (!timestamp) {
            const userInput = prompt(`Could not extract timestamp from filename "${file.name}".\n\nEnter the encryption timestamp (or leave empty to try recent hashes):\n\nTip: Check your encryption records below for the correct timestamp.`);
            if (userInput) {
                timestamp = parseInt(userInput);
                if (isNaN(timestamp)) {
                    this.showNotification('Invalid timestamp format. Please enter a valid number.', 'error');
                    return;
                }
            }
        } else {
            this.showNotification(`Extracted timestamp ${timestamp} from filename`, 'info');
        }

        try {
            this.showNotification('Decrypting file...', 'info');
            
            let decryptedBlob;
            if (timestamp) {
                decryptedBlob = await this.antCrypt.decryptFile(file, timestamp);
            } else {
                // Try recent hashes if no timestamp provided
                decryptedBlob = await this.tryRecentHashes(file);
            }
            
            // Create download link
            const originalName = this.getOriginalFileName(file.name);
            const url = URL.createObjectURL(decryptedBlob);
            const link = document.createElement('a');
            link.href = url;
            link.download = originalName;
            link.click();
            URL.revokeObjectURL(url);

            this.showNotification('File decrypted successfully!', 'success');
            
            // Reset file input
            fileInput.value = '';
            document.getElementById('decryptFileName').textContent = 'No file selected';
            document.getElementById('decryptButton').disabled = true;

        } catch (error) {
            console.error('Decryption error:', error);
            this.showNotification(`Decryption failed: ${error.message}`, 'error');
        }
    }

    async tryRecentHashes(file) {
        const recentHashes = this.antCrypt.hashHistory.slice(0, 100);
        
        for (const hashEntry of recentHashes) {
            try {
                return await this.antCrypt.decryptFile(file, hashEntry.timestamp);
            } catch (error) {
                // Continue to next hash
                continue;
            }
        }
        
        throw new Error('Could not decrypt with any recent hashes. Please provide the correct timestamp.');
    }

    extractTimestampFromFilename(filename) {
        // Try to extract timestamp from filename patterns like encrypted_1234567890123_file.txt.ant
        const match = filename.match(/encrypted_(\d{13})_/); // 13-digit timestamp
        return match ? parseInt(match[1]) : null;
    }

    getOriginalFileName(encryptedName) {
        // Remove encrypted_timestamp_ prefix and .ant extension
        return encryptedName.replace(/^encrypted_\d{13}_/, '').replace(/\.ant$/, '');
    }

    updateRecordsDisplay() {
        const container = document.getElementById('encryptionRecords');
        if (!container) return;

        if (this.encryptionRecords.length === 0) {
            container.innerHTML = '<p class="has-text-grey">No encryption records yet...</p>';
            return;
        }

        const recordsHTML = this.encryptionRecords.map(record => `
            <div class="box" style="margin-bottom: 10px; padding: 15px; background: rgba(255, 255, 255, 0.05);">
                <div class="columns is-mobile">
                    <div class="column">
                        <p><strong>📁 ${record.originalFileName}</strong></p>
                        <p class="is-size-7 has-text-grey">Encrypted: ${record.dateCreated}</p>
                        <p class="is-size-7 has-text-grey">Size: ${this.formatFileSize(record.fileSize)}</p>
                    </div>
                    <div class="column">
                        <p class="is-size-7"><strong>🔐 Hash:</strong></p>
                        <code class="is-size-7" style="word-break: break-all;">${record.hash.substring(0, 32)}...</code>
                        <p class="is-size-7"><strong>⏰ Timestamp:</strong> ${record.timestamp}</p>
                    </div>
                    <div class="column is-narrow">
                        <button class="button is-small is-info" onclick="encryptionUI.copyDecryptionInfo('${record.hash}', ${record.timestamp})">
                            📋 Copy Info
                        </button>
                        <button class="button is-small is-danger" onclick="encryptionUI.deleteRecord(${record.id})">
                            🗑️
                        </button>
                    </div>
                </div>
            </div>
        `).join('');

        container.innerHTML = recordsHTML;
    }

    copyDecryptionInfo(hash, timestamp) {
        const info = `Decryption Info:\nHash: ${hash}\nTimestamp: ${timestamp}\nDate: ${new Date(timestamp).toLocaleString()}`;
        navigator.clipboard.writeText(info).then(() => {
            this.showNotification('Decryption info copied to clipboard!', 'success');
        }).catch(() => {
            // Fallback for older browsers
            const textArea = document.createElement('textarea');
            textArea.value = info;
            document.body.appendChild(textArea);
            textArea.select();
            document.execCommand('copy');
            document.body.removeChild(textArea);
            this.showNotification('Decryption info copied to clipboard!', 'success');
        });
    }

    deleteRecord(id) {
        this.encryptionRecords = this.encryptionRecords.filter(record => record.id !== id);
        this.saveRecords();
        this.updateRecordsDisplay();
        this.showNotification('Record deleted', 'info');
    }

    clearRecords() {
        if (confirm('Are you sure you want to clear all encryption records?')) {
            this.encryptionRecords = [];
            this.saveRecords();
            this.updateRecordsDisplay();
            this.showNotification('All records cleared', 'info');
        }
    }

    saveRecords() {
        localStorage.setItem('antEncryptionRecords', JSON.stringify(this.encryptionRecords));
    }

    formatFileSize(bytes) {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    showNotification(message, type = 'info') {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification is-${type} is-light`;
        notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 9999;
            max-width: 400px;
            animation: slideIn 0.3s ease-out;
        `;
        
        notification.innerHTML = `
            <button class="delete"></button>
            ${message}
        `;

        // Add close functionality
        notification.querySelector('.delete').addEventListener('click', () => {
            notification.remove();
        });

        document.body.appendChild(notification);

        // Auto-remove after 5 seconds
        setTimeout(() => {
            if (notification.parentNode) {
                notification.remove();
            }
        }, 5000);
    }
}

// Add CSS for notification animation
const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(100%);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }
`;
document.head.appendChild(style);

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.encryptionUI = new EncryptionUI();
});

console.log('🔐 Encryption UI initialized'); 