const CACHE = 'windterm-v1';
const ASSETS = ['/', '/index.html', '/styles.css', '/manifest.webmanifest'];

self.addEventListener('install', (e: any) => {
  e.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(ASSETS)));
});

self.addEventListener('fetch', (e: any) => {
  e.respondWith(
    caches.match(e.request).then((r) => r || fetch(e.request)).catch(() => caches.match('/index.html'))
  );
});
