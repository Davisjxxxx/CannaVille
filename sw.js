/**
 * CannaVille Pro - Service Worker
 * Provides offline functionality and caching for PWA
 */

const CACHE_NAME = 'cannaville-pro-v1.0.0';
const STATIC_CACHE = 'cannaville-static-v1';
const DYNAMIC_CACHE = 'cannaville-dynamic-v1';

// Files to cache for offline functionality
const STATIC_FILES = [
    '/',
    '/index.html',
    '/src/main.js',
    '/src/core/Engine.js',
    '/src/systems/PlantSystem.js',
    '/src/systems/EnvironmentSystem.js',
    '/src/systems/InputSystem.js',
    '/src/systems/FirstPersonController.js',
    '/src/systems/ShaderSystem.js',
    '/src/systems/LightingSystem.js',
    '/src/ui/UISystem.js',
    '/manifest.json',
    // Add Three.js and other essential libraries
    '/node_modules/three/build/three.module.js',
    '/node_modules/gsap/dist/gsap.min.js'
];

// API endpoints to cache
const API_CACHE_PATTERNS = [
    /\/api\/auth\/profile/,
    /\/api\/plants/,
    /\/api\/game\/saves/,
    /\/api\/environment/,
    /\/api\/stats/
];

// Install event - cache static files
self.addEventListener('install', (event) => {
    console.log('🔧 Service Worker installing...');
    
    event.waitUntil(
        caches.open(STATIC_CACHE)
            .then((cache) => {
                console.log('📦 Caching static files...');
                return cache.addAll(STATIC_FILES);
            })
            .then(() => {
                console.log('✅ Static files cached successfully');
                return self.skipWaiting();
            })
            .catch((error) => {
                console.error('❌ Failed to cache static files:', error);
            })
    );
});

// Activate event - clean up old caches
self.addEventListener('activate', (event) => {
    console.log('🚀 Service Worker activating...');
    
    event.waitUntil(
        caches.keys()
            .then((cacheNames) => {
                return Promise.all(
                    cacheNames.map((cacheName) => {
                        if (cacheName !== STATIC_CACHE && cacheName !== DYNAMIC_CACHE) {
                            console.log('🗑️ Deleting old cache:', cacheName);
                            return caches.delete(cacheName);
                        }
                    })
                );
            })
            .then(() => {
                console.log('✅ Service Worker activated');
                return self.clients.claim();
            })
    );
});

// Fetch event - serve cached content and implement caching strategies
self.addEventListener('fetch', (event) => {
    const { request } = event;
    const url = new URL(request.url);
    
    // Skip non-GET requests
    if (request.method !== 'GET') {
        return;
    }
    
    // Skip chrome-extension and other non-http(s) requests
    if (!url.protocol.startsWith('http')) {
        return;
    }
    
    // Handle different types of requests
    if (isStaticFile(request.url)) {
        // Static files - Cache First strategy
        event.respondWith(cacheFirst(request));
    } else if (isAPIRequest(request.url)) {
        // API requests - Network First strategy
        event.respondWith(networkFirst(request));
    } else if (isAssetFile(request.url)) {
        // Assets (models, textures) - Cache First strategy
        event.respondWith(cacheFirst(request));
    } else {
        // Other requests - Network First with fallback
        event.respondWith(networkFirst(request));
    }
});

// Cache First strategy - good for static files
async function cacheFirst(request) {
    try {
        const cachedResponse = await caches.match(request);
        if (cachedResponse) {
            return cachedResponse;
        }
        
        const networkResponse = await fetch(request);
        
        // Cache successful responses
        if (networkResponse.status === 200) {
            const cache = await caches.open(STATIC_CACHE);
            cache.put(request, networkResponse.clone());
        }
        
        return networkResponse;
    } catch (error) {
        console.error('Cache First failed:', error);
        
        // Return offline fallback for HTML requests
        if (request.headers.get('accept').includes('text/html')) {
            return caches.match('/offline.html') || new Response(
                '<h1>Offline</h1><p>Please check your internet connection.</p>',
                { headers: { 'Content-Type': 'text/html' } }
            );
        }
        
        throw error;
    }
}

// Network First strategy - good for API requests
async function networkFirst(request) {
    try {
        const networkResponse = await fetch(request);
        
        // Cache successful API responses
        if (networkResponse.status === 200 && isAPIRequest(request.url)) {
            const cache = await caches.open(DYNAMIC_CACHE);
            cache.put(request, networkResponse.clone());
        }
        
        return networkResponse;
    } catch (error) {
        console.log('Network failed, trying cache:', request.url);
        
        const cachedResponse = await caches.match(request);
        if (cachedResponse) {
            return cachedResponse;
        }
        
        // Return offline response for API requests
        if (isAPIRequest(request.url)) {
            return new Response(
                JSON.stringify({ 
                    error: 'Offline', 
                    message: 'This feature requires an internet connection.' 
                }),
                { 
                    status: 503,
                    headers: { 'Content-Type': 'application/json' } 
                }
            );
        }
        
        throw error;
    }
}

// Helper functions
function isStaticFile(url) {
    return STATIC_FILES.some(file => url.includes(file)) ||
           url.includes('.js') ||
           url.includes('.css') ||
           url.includes('.html');
}

function isAPIRequest(url) {
    return url.includes('/api/') ||
           API_CACHE_PATTERNS.some(pattern => pattern.test(url));
}

function isAssetFile(url) {
    return url.includes('.glb') ||
           url.includes('.gltf') ||
           url.includes('.jpg') ||
           url.includes('.png') ||
           url.includes('.webp') ||
           url.includes('.mp3') ||
           url.includes('.wav');
}

// Background sync for offline actions
self.addEventListener('sync', (event) => {
    console.log('🔄 Background sync triggered:', event.tag);
    
    if (event.tag === 'plant-update') {
        event.waitUntil(syncPlantUpdates());
    } else if (event.tag === 'game-save') {
        event.waitUntil(syncGameSaves());
    }
});

// Sync plant updates when back online
async function syncPlantUpdates() {
    try {
        const pendingUpdates = await getStoredData('pendingPlantUpdates');
        
        for (const update of pendingUpdates) {
            try {
                await fetch('/api/plants/' + update.plantId, {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json',
                        'Authorization': update.token
                    },
                    body: JSON.stringify(update.data)
                });
                
                console.log('✅ Synced plant update:', update.plantId);
            } catch (error) {
                console.error('❌ Failed to sync plant update:', error);
            }
        }
        
        // Clear synced updates
        await clearStoredData('pendingPlantUpdates');
    } catch (error) {
        console.error('❌ Background sync failed:', error);
    }
}

// Sync game saves when back online
async function syncGameSaves() {
    try {
        const pendingSaves = await getStoredData('pendingGameSaves');
        
        for (const save of pendingSaves) {
            try {
                const response = await fetch('/api/game/saves/' + save.saveId, {
                    method: 'PUT',
                    headers: {
                        'Content-Type': 'application/json',
                        'Authorization': save.token
                    },
                    body: JSON.stringify(save.data)
                });
                
                if (response.ok) {
                    console.log('✅ Synced game save:', save.saveId);
                }
            } catch (error) {
                console.error('❌ Failed to sync game save:', error);
            }
        }
        
        // Clear synced saves
        await clearStoredData('pendingGameSaves');
    } catch (error) {
        console.error('❌ Background sync failed:', error);
    }
}

// Push notifications
self.addEventListener('push', (event) => {
    console.log('📱 Push notification received');
    
    const options = {
        body: 'Your plants need attention!',
        icon: '/icons/icon-192x192.png',
        badge: '/icons/badge-72x72.png',
        vibrate: [200, 100, 200],
        data: {
            url: '/',
            action: 'check-plants'
        },
        actions: [
            {
                action: 'check-plants',
                title: 'Check Plants',
                icon: '/icons/plant-icon.png'
            },
            {
                action: 'dismiss',
                title: 'Dismiss',
                icon: '/icons/dismiss-icon.png'
            }
        ]
    };
    
    if (event.data) {
        const data = event.data.json();
        options.body = data.message || options.body;
        options.data = { ...options.data, ...data };
    }
    
    event.waitUntil(
        self.registration.showNotification('CannaVille Pro', options)
    );
});

// Handle notification clicks
self.addEventListener('notificationclick', (event) => {
    console.log('🔔 Notification clicked:', event.action);
    
    event.notification.close();
    
    if (event.action === 'check-plants') {
        event.waitUntil(
            clients.openWindow('/?action=check-plants')
        );
    } else if (event.action !== 'dismiss') {
        event.waitUntil(
            clients.openWindow('/')
        );
    }
});

// Utility functions for IndexedDB storage
async function getStoredData(key) {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open('CannaVillePro', 1);
        
        request.onerror = () => reject(request.error);
        
        request.onsuccess = () => {
            const db = request.result;
            const transaction = db.transaction(['offline'], 'readonly');
            const store = transaction.objectStore('offline');
            const getRequest = store.get(key);
            
            getRequest.onsuccess = () => {
                resolve(getRequest.result?.data || []);
            };
            
            getRequest.onerror = () => reject(getRequest.error);
        };
        
        request.onupgradeneeded = () => {
            const db = request.result;
            if (!db.objectStoreNames.contains('offline')) {
                db.createObjectStore('offline', { keyPath: 'key' });
            }
        };
    });
}

async function storeData(key, data) {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open('CannaVillePro', 1);
        
        request.onerror = () => reject(request.error);
        
        request.onsuccess = () => {
            const db = request.result;
            const transaction = db.transaction(['offline'], 'readwrite');
            const store = transaction.objectStore('offline');
            const putRequest = store.put({ key, data });
            
            putRequest.onsuccess = () => resolve();
            putRequest.onerror = () => reject(putRequest.error);
        };
    });
}

async function clearStoredData(key) {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open('CannaVillePro', 1);
        
        request.onerror = () => reject(request.error);
        
        request.onsuccess = () => {
            const db = request.result;
            const transaction = db.transaction(['offline'], 'readwrite');
            const store = transaction.objectStore('offline');
            const deleteRequest = store.delete(key);
            
            deleteRequest.onsuccess = () => resolve();
            deleteRequest.onerror = () => reject(deleteRequest.error);
        };
    });
}

// Message handling for communication with main thread
self.addEventListener('message', (event) => {
    console.log('📨 Service Worker received message:', event.data);
    
    if (event.data.type === 'SKIP_WAITING') {
        self.skipWaiting();
    } else if (event.data.type === 'CACHE_PLANT_UPDATE') {
        storeData('pendingPlantUpdates', event.data.updates);
    } else if (event.data.type === 'CACHE_GAME_SAVE') {
        storeData('pendingGameSaves', event.data.saves);
    }
});

console.log('🌿 CannaVille Pro Service Worker loaded successfully');

