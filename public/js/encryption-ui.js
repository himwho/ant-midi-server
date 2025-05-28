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
        const hashInput = document.getElementById('decryptHashInput');
        const file = fileInput.files[0];
        
        if (!file) {
            this.showNotification('Please select a file to decrypt', 'error');
            return;
        }

        // Check if user provided a manual hash
        const manualHash = hashInput.value.trim();
        let timestamp = null;
        let useManualHash = false;

        if (manualHash) {
            // Validate hash format (should be 64 character hex string)
            if (!/^[a-fA-F0-9]{64}$/.test(manualHash)) {
                this.showNotification('Invalid hash format. Hash should be 64 hexadecimal characters.', 'error');
                return;
            }
            useManualHash = true;
            this.showNotification('Using manually provided hash for decryption', 'info');
        } else {
            // Try to extract timestamp from filename
            timestamp = this.extractTimestampFromFilename(file.name);
            
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
        }

        try {
            this.showNotification('Decrypting file...', 'info');
            
            let decryptedBlob;
            if (useManualHash) {
                // Use manual hash for decryption
                decryptedBlob = await this.decryptWithManualHash(file, manualHash);
            } else if (timestamp) {
                // Use timestamp-based lookup
                decryptedBlob = await this.antCrypt.decryptFile(file, timestamp);
            } else {
                // Try recent hashes if no timestamp provided
                decryptedBlob = await this.tryRecentHashes(file);
            }
            
            // Show file preview for text files
            await this.showFilePreview(decryptedBlob, file.name);
            
            // Create download link
            const originalName = this.getOriginalFileName(file.name);
            const url = URL.createObjectURL(decryptedBlob);
            const link = document.createElement('a');
            link.href = url;
            link.download = originalName;
            
            // Add the link to the document temporarily
            document.body.appendChild(link);
            
            // Show notification before download
            this.showNotification(`Decryption successful! Downloading: ${originalName}`, 'success');
            
            // Add download status to the page
            this.addDownloadStatus(originalName, url, link, decryptedBlob);
            
            // Create a manual download button as backup
            const manualDownloadBtn = document.createElement('button');
            manualDownloadBtn.className = 'button is-primary';
            manualDownloadBtn.innerHTML = `📥 Download ${originalName}`;
            manualDownloadBtn.onclick = () => {
                link.click();
                manualDownloadBtn.remove();
            };
            
            // Trigger download with a slight delay to ensure notification shows
            setTimeout(() => {
                try {
                    // Try multiple download approaches
                    console.log('Attempting download for:', originalName);
                    console.log('Download URL:', url);
                    console.log('File size:', decryptedBlob.size, 'bytes');
                    
                    // Method 1: Standard click
                    link.click();
                    console.log('Standard download triggered');
                    
                    // Don't remove the link immediately - keep it for manual downloads
                    // document.body.removeChild(link);
                    
                    // Show additional notification about download
                    setTimeout(() => {
                        this.showNotification(`Download initiated for "${originalName}". Check your Downloads folder or browser's download manager.`, 'info');
                        
                        // Also show browser-specific instructions
                        setTimeout(() => {
                            this.showNotification('💡 If download didn\'t work: Use the manual download buttons in the blue section below, or check browser permissions.', 'warning');
                        }, 3000);
                    }, 1000);
                } catch (error) {
                    console.error('Automatic download failed:', error);
                    this.showNotification('Automatic download failed. Use the manual download buttons below:', 'warning');
                }
                
                // Don't revoke URL immediately - keep it for manual downloads
                // The URL will be revoked when the download status section is removed
            }, 500);

            // Reset inputs
            fileInput.value = '';
            hashInput.value = '';
            document.getElementById('decryptFileName').textContent = 'No file selected';
            document.getElementById('decryptButton').disabled = true;

        } catch (error) {
            console.error('Decryption error:', error);
            this.showNotification(`Decryption failed: ${error.message}`, 'error');
        }
    }

    async decryptWithManualHash(file, hash) {
        // Create a temporary hash entry for decryption
        const tempHashEntry = {
            hash: hash,
            timestamp: Date.now() // Use current time as placeholder
        };
        
        // Temporarily add to hash history for decryption
        this.antCrypt.hashHistory.unshift(tempHashEntry);
        
        try {
            // Use the AntCrypt decryptFile method with the temp timestamp
            const decryptedBlob = await this.antCrypt.decryptFile(file, tempHashEntry.timestamp);
            return decryptedBlob;
        } finally {
            // Remove the temporary hash entry
            this.antCrypt.hashHistory.shift();
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
                        <button class="button is-small is-primary" onclick="encryptionUI.copyHashOnly('${record.hash}')">
                            🔐 Copy Hash
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
        const info = `🔐 Ant Colony Decryption Info:

Hash: ${hash}
Timestamp: ${timestamp}
Date: ${new Date(timestamp).toLocaleString()}

📋 Quick Copy (Hash Only):
${hash}

💡 Instructions:
1. Share the encrypted file (.ant) with someone
2. Also share this hash with them
3. They can paste the hash in the "Decryption Hash" field
4. Or use the timestamp if they prefer automatic lookup`;

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

    copyHashOnly(hash) {
        navigator.clipboard.writeText(hash).then(() => {
            this.showNotification('Hash copied to clipboard! Paste it in the decryption field.', 'success');
        }).catch(() => {
            // Fallback for older browsers
            const textArea = document.createElement('textarea');
            textArea.value = hash;
            document.body.appendChild(textArea);
            textArea.select();
            document.execCommand('copy');
            document.body.removeChild(textArea);
            this.showNotification('Hash copied to clipboard! Paste it in the decryption field.', 'success');
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

    async showFilePreview(decryptedBlob, originalFilename) {
        const originalName = this.getOriginalFileName(originalFilename);
        
        // Check if it's likely a text file
        const textExtensions = ['.txt', '.md', '.json', '.csv', '.log', '.xml', '.html', '.css', '.js', '.py', '.java', '.cpp', '.c', '.h'];
        const isTextFile = textExtensions.some(ext => originalName.toLowerCase().endsWith(ext));
        
        if (isTextFile && decryptedBlob.size < 10000) { // Only preview small text files
            try {
                const text = await decryptedBlob.text();
                const preview = text.length > 200 ? text.substring(0, 200) + '...' : text;
                
                // Create preview notification
                const previewDiv = document.createElement('div');
                previewDiv.className = 'notification is-success';
                previewDiv.innerHTML = `
                    <button class="delete"></button>
                    <h5><strong>📄 Decrypted File Preview: ${originalName}</strong></h5>
                    <pre style="background: rgba(0,0,0,0.1); padding: 10px; border-radius: 4px; margin-top: 10px; white-space: pre-wrap; font-size: 0.9em;">${this.escapeHtml(preview)}</pre>
                    <p style="margin-top: 10px;"><em>File size: ${this.formatFileSize(decryptedBlob.size)}</em></p>
                `;
                
                // Add close functionality
                previewDiv.querySelector('.delete').addEventListener('click', () => {
                    previewDiv.remove();
                });
                
                // Add to page
                const container = document.querySelector('.encryption-section .container');
                if (container) {
                    container.appendChild(previewDiv);
                    
                    // Auto-remove after 15 seconds
                    setTimeout(() => {
                        if (previewDiv.parentNode) {
                            previewDiv.remove();
                        }
                    }, 15000);
                }
            } catch (error) {
                console.log('Could not preview file as text:', error);
            }
        }
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    addDownloadStatus(originalName, url, link, decryptedBlob) {
        // Remove any existing download status
        const existingStatus = document.getElementById('downloadStatus');
        if (existingStatus) {
            existingStatus.remove();
        }
        
        // Create download status section
        const statusDiv = document.createElement('div');
        statusDiv.id = 'downloadStatus';
        statusDiv.className = 'notification is-info';
        statusDiv.innerHTML = `
            <button class="delete"></button>
            <h5><strong>📥 Download Ready: ${originalName}</strong></h5>
            <p>Your decrypted file is ready for download.</p>
            <div style="margin-top: 15px;">
                <button class="button is-primary" id="manualDownloadBtn">
                    📥 Download ${originalName}
                </button>
                <button class="button is-light" id="saveAsBtn">
                    💾 Save As...
                </button>
                <button class="button is-info" id="copyContentBtn" style="display: none;">
                    📋 Copy Content
                </button>
            </div>
            <p style="margin-top: 10px; font-size: 0.9em; color: #666;">
                💡 If the download doesn't start automatically, use the buttons above.
            </p>
        `;
        
        // Add event listeners
        statusDiv.querySelector('.delete').addEventListener('click', () => {
            statusDiv.remove();
            URL.revokeObjectURL(url);
        });
        
        // Show copy content button for text files
        const textExtensions = ['.txt', '.md', '.json', '.csv', '.log', '.xml', '.html', '.css', '.js', '.py', '.java', '.cpp', '.c', '.h'];
        const isTextFile = textExtensions.some(ext => originalName.toLowerCase().endsWith(ext));
        if (isTextFile && decryptedBlob.size < 50000) { // Show for text files under 50KB
            statusDiv.querySelector('#copyContentBtn').style.display = 'inline-block';
        }
        
        statusDiv.querySelector('#manualDownloadBtn').addEventListener('click', () => {
            console.log('Manual download button clicked');
            console.log('Download URL:', url);
            
            // Try multiple download methods
            try {
                // Method 1: Direct link click
                link.click();
                this.showNotification('Download started via manual button!', 'success');
            } catch (error) {
                console.error('Manual download method 1 failed:', error);
                
                // Method 2: Create new link and click
                try {
                    const newLink = document.createElement('a');
                    newLink.href = url;
                    newLink.download = originalName;
                    newLink.style.display = 'none';
                    document.body.appendChild(newLink);
                    newLink.click();
                    document.body.removeChild(newLink);
                    this.showNotification('Download started via backup method!', 'success');
                } catch (error2) {
                    console.error('Manual download method 2 failed:', error2);
                    this.showNotification('Manual download failed. Try the "Save As" button or check browser console for errors.', 'error');
                }
            }
        });
        
        statusDiv.querySelector('#saveAsBtn').addEventListener('click', () => {
            console.log('Save As button clicked');
            
            try {
                // Method 1: Open in new tab (user can then save)
                const newWindow = window.open(url, '_blank');
                if (newWindow) {
                    this.showNotification('File opened in new tab. Use Ctrl+S (Cmd+S on Mac) to save it.', 'info');
                } else {
                    throw new Error('Popup blocked');
                }
            } catch (error) {
                console.error('Save As method 1 failed:', error);
                
                // Method 2: Try direct download with different approach
                try {
                    const saveLink = document.createElement('a');
                    saveLink.href = url;
                    saveLink.download = originalName;
                    saveLink.target = '_blank';
                    saveLink.rel = 'noopener noreferrer';
                    document.body.appendChild(saveLink);
                    
                    // Force click with user gesture
                    const clickEvent = new MouseEvent('click', {
                        view: window,
                        bubbles: true,
                        cancelable: true,
                        buttons: 1
                    });
                    saveLink.dispatchEvent(clickEvent);
                    
                    document.body.removeChild(saveLink);
                    this.showNotification('Save As attempted. Check your browser\'s download manager.', 'info');
                } catch (error2) {
                    console.error('Save As method 2 failed:', error2);
                    this.showNotification('Save As failed. Try right-clicking the "Download" button and selecting "Save link as..."', 'warning');
                }
            }
        });
        
        statusDiv.querySelector('#copyContentBtn').addEventListener('click', async () => {
            console.log('Copy content button clicked');
            
            try {
                // Method 1: Copy content to clipboard
                const text = await decryptedBlob.text();
                navigator.clipboard.writeText(text).then(() => {
                    this.showNotification('Content copied to clipboard!', 'success');
                }).catch(() => {
                    // Fallback for older browsers
                    const textArea = document.createElement('textarea');
                    textArea.value = text;
                    document.body.appendChild(textArea);
                    textArea.select();
                    document.execCommand('copy');
                    document.body.removeChild(textArea);
                    this.showNotification('Content copied to clipboard!', 'success');
                });
            } catch (error) {
                console.error('Copy content method failed:', error);
                this.showNotification('Copy content failed. Please try the manual download buttons below.', 'error');
            }
        });
        
        // Add to page
        const container = document.querySelector('.encryption-section .container');
        if (container) {
            container.appendChild(statusDiv);
        }
        
        // Auto-remove after 2 minutes
        setTimeout(() => {
            if (statusDiv.parentNode) {
                statusDiv.remove();
                URL.revokeObjectURL(url);
            }
        }, 120000);
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