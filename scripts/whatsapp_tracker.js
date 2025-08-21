// WhatsApp Durum Takip, Bağlantı Testi ve Raporlama Scripti v2.1
// Bu dosya, CGPT.cpp tarafından okunup çalıştırılır.
// Değişiklik: forceKeepAlive fonksiyonuna detaylı loglama ve daha gerçekçi event dispatch eklendi.

(function() {
    try {
        const post = (msg) => window.chrome.webview.postMessage(msg);
        post('DETAIL:Ana script v2.1 çalıştırıldı.');

        // Scriptin yeniden çalıştırılması durumunda önceki takipçileri temizle
        if (window.whatsAppTracker) {
            window.whatsAppTracker.stop();
            post('DETAIL:Önceki takip ve testler durduruldu.');
        }

        window.whatsAppTracker = {
            isOnline: false,
            lastActivity: '',
            periodicCheckCounter: 0,
            connectionTestResults: [],

            observer: null,
            statusInterval: null,
            connectionInterval: null,
            minuteLogInterval: null,
            keepAliveInterval: null,

            // Element seçicileri
            HEADER_SELECTOR: '#main > header',
            ONLINE_SELECTOR: 'span[title="çevrimiçi"]',
            ACTIVITY_SELECTOR: 'span[title="yazıyor..."], span[title="kaydediyor..."]',
            RECONNECT_BUTTON_SELECTOR: 'button[class*="xjb2p0i"]',
            CONNECTION_ERROR_PANEL_SELECTOR: '#side span.x78zum5.x1c4vz4f',
            CHAT_BACKGROUND_SELECTOR: 'div[data-testid="conversation-panel-body"]',

            // Ana başlatma fonksiyonu
            start: function() {
                post('DETAIL:Takip sistemi başlatılıyor...');
                const headerElement = document.querySelector(this.HEADER_SELECTOR);
                if (!headerElement) {
                    post('ERROR:Aktif sohbet başlığı bulunamadı. Lütfen bir sohbet penceresi açtığınızdan emin olun.');
                    return;
                }
                post('DETAIL:Aktif sohbet başlığı bulundu.');

                this.observer = new MutationObserver(() => this.checkStatus(headerElement));
                this.observer.observe(headerElement, { childList: true, subtree: true, characterData: true, attributes: true });
                post('DETAIL:MutationObserver (anlık takip) başarıyla başlatıldı.');

                this.statusInterval = setInterval(() => {
                    this.periodicCheckCounter++;
                    this.checkStatus(headerElement);
                }, 1000);
                post('DETAIL:Periyodik durum kontrolü (saniyede 1) başarıyla başlatıldı.');

                this.connectionInterval = setInterval(() => this.runConnectionTests(), 30000);
                post('DETAIL:Bağlantı test döngüsü (30 saniyede 1) başarıyla başlatıldı.');

                this.minuteLogInterval = setInterval(() => this.logMinuteSummary(), 60000);
                post('DETAIL:Dakikalık özet loglama (60 saniyede 1) başarıyla başlatıldı.');

                this.keepAliveInterval = setInterval(() => this.forceKeepAlive(), 5000);
                post('DETAIL:Bağlantı Canlı Tutma (Keep-Alive) 5 saniyede bir devrede.');

                this.checkStatus(headerElement);
                this.runConnectionTests();
            },

            // Tüm takip mekanizmalarını durdurur
            stop: function() {
                if (this.observer) this.observer.disconnect();
                if (this.statusInterval) clearInterval(this.statusInterval);
                if (this.connectionInterval) clearInterval(this.connectionInterval);
                if (this.minuteLogInterval) clearInterval(this.minuteLogInterval);
                if (this.keepAliveInterval) clearInterval(this.keepAliveInterval);
            },

            // Çevrimiçi, çevrimdışı ve diğer aktiviteleri kontrol eder
            checkStatus: function(header) {
                const onlineElement = header.querySelector(this.ONLINE_SELECTOR);
                let currentActivity = 'offline';
                if(onlineElement) {
                    currentActivity = 'online';
                } else {
                    const activityElement = header.querySelector(this.ACTIVITY_SELECTOR);
                    if(activityElement) {
                        currentActivity = activityElement.getAttribute('title');
                    }
                }

                if (currentActivity !== this.lastActivity) {
                    post('WHATSAPP_ACTIVITY:' + currentActivity);
                    this.lastActivity = currentActivity;
                }
            },

            // 5 aşamalı bağlantı testini çalıştırır
            runConnectionTests: async function() {
                let results = [];
                // Test 1: Ping Testi
                try {
                    const response = await fetch('https://static.whatsapp.net/rsrc.php/v3/y6/r/wa669ae9gAW.png?t=' + new Date().getTime(), { method: 'HEAD', cache: 'no-store' });
                    results.push('  - 1. Ping Testi: BAŞARILI (Sunucuya ulaşıldı, HTTP ' + response.status + ')');
                } catch (e) {
                    results.push('  - 1. Ping Testi: BAŞARISIZ (Sunucuya ulaşılamadı veya ağ hatası)');
                }

                // Test 2: Veri Alışverişi Testi
                const initialResourceCount = performance.getEntriesByType('resource').filter(r => r.name.includes('whatsapp.net')).length;
                await new Promise(resolve => setTimeout(resolve, 10000));
                const finalResourceCount = performance.getEntriesByType('resource').filter(r => r.name.includes('whatsapp.net')).length;
                if (finalResourceCount > initialResourceCount) {
                    results.push('  - 2. Veri Alışverişi: BAŞARILI (' + (finalResourceCount - initialResourceCount) + ' yeni kaynak yüklendi)');
                } else {
                    results.push('  - 2. Veri Alışverişi: BAŞARISIZ (10 saniyede yeni kaynak yüklenmedi)');
                }

                // Test 3 & 5: WebSocket ve Bağlantı Uyarısı Kontrolü
                const errorPanel = document.querySelector(this.CONNECTION_ERROR_PANEL_SELECTOR);
                let isErrorVisible = false;
                if (errorPanel && window.getComputedStyle(errorPanel).display !== 'none') {
                    const errorText = errorPanel.innerText.toLowerCase();
                    if (errorText.includes('bilgisayar bağlı değil') || errorText.includes('tekrar bağlan')) {
                        isErrorVisible = true;
                    }
                }
                results.push('  - 3. WebSocket Sağlığı: ' + (isErrorVisible ? 'BAŞARISIZ (Bağlantı hatası uyarısı görüldü)' : 'BAŞARILI (Arayüzde bağlantı hatası yok)'));
                results.push('  - 5. Uyarı Etiketi Kontrolü: ' + (isErrorVisible ? 'MEVCUT' : 'MEVCUT DEĞİL'));


                // Test 4: Bağlantı Canlandırma
                try {
                    const headerElement = document.querySelector(this.HEADER_SELECTOR);
                    if (headerElement) {
                        headerElement.dispatchEvent(new MouseEvent('mouseover', { bubbles: true }));
                        await new Promise(resolve => setTimeout(resolve, 200));
                        headerElement.dispatchEvent(new MouseEvent('mouseout', { bubbles: true }));
                        results.push('  - 4. Bağlantı Canlandırma: BAŞARILI (Sohbet başlığına mouseover simülasyonu yapıldı)');
                    } else {
                        results.push('  - 4. Bağlantı Canlandırma: BAŞARISIZ (Sohbet başlığı bulunamadı)');
                    }
                } catch (e) {
                    results.push('  - 4. Bağlantı Canlandırma: BAŞARISIZ (Simülasyon sırasında hata: ' + e.message + ')');
                }

                this.connectionTestResults.push(results.join('\r\n'));
            },

            // "forced-brute-click" mantığı
            forceKeepAlive: function() {
                const chatBg = document.querySelector(this.CHAT_BACKGROUND_SELECTOR);
                if (chatBg) {
                    post('DETAIL:Keep-alive: Found element with selector: ' + this.CHAT_BACKGROUND_SELECTOR + '. Simulating click.');
                    chatBg.dispatchEvent(new MouseEvent('mousedown', { bubbles: true, composed: true, view: window, cancelable: true, buttons: 1 }));
                    chatBg.dispatchEvent(new MouseEvent('mouseup', { bubbles: true, composed: true, view: window, cancelable: true, buttons: 1 }));
                } else {
                    post('ERROR:Keep-alive: Could not find element with selector: ' + this.CHAT_BACKGROUND_SELECTOR);
                }
            },

            // Dakikalık özet raporunu C++'a gönderir
            logMinuteSummary: function() {
                let summary = `Son 1 dakikalık rapor:\r\n- ${this.periodicCheckCounter} periyodik durum kontrolü yapıldı.\r\n- Bağlantı Testleri Sonuçları:\r\n`;
                if (this.connectionTestResults.length > 0) {
                    summary += this.connectionTestResults.join('\r\n---\r\n');
                } else {
                    summary += "  (Henüz tamamlanmış test döngüsü yok)";
                }
                post('LOG_SUMMARY:' + summary);
                this.periodicCheckCounter = 0;
                this.connectionTestResults = [];
            }
        };

        window.whatsAppTracker.start();

    } catch (e) {
        window.chrome.webview.postMessage('ERROR:Genel script hatası: ' + e.message);
    }
})();
