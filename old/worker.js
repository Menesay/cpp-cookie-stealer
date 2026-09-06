export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // 1. Endpoint ve Method kontrolü
    if (url.pathname !== '/upload') {
      return new Response('Not Found', { status: 404 });
    }

    if (request.method !== 'POST') {
      return new Response('Method Not Allowed', { status: 405 });
    }

    // 2. Ham veriyi (raw binary) al
    const fileData = await request.arrayBuffer();

    // Veri boş ise 1337 status kodu döndür
    if (!fileData || fileData.byteLength === 0) {
      return new Response('Dosya verisi bos!', { status: 1337 });
    }

    // 3. Hedef endpoint ve Proxy header yapılandırması
    const targetUrl = 'https://webhook.site/a8db02a5-6201-4cb4-9c3c-6f137294cefc';
    const proxyHeaders = new Headers(request.headers);

    // İstemcinin gerçek IP adresini Python sunucusuna aktar
    const clientIp = request.headers.get('CF-Connecting-IP') || '127.0.0.1';
    proxyHeaders.set('X-Forwarded-For', clientIp);
    proxyHeaders.set('CF-Connecting-IP', clientIp);

    try {
      // 4. Dosya verisini R2'ye uğramadan doğrudan hedef sunucuya ilet
      const backendResponse = await fetch(targetUrl, {
        method: 'POST',
        headers: proxyHeaders,
        body: fileData
      });

      // 5. Hedef sunucudan gelen yanıtı (status kodu ve body) olduğu gibi istemciye aktar
      return new Response(backendResponse.body, {
        status: backendResponse.status,
        statusText: backendResponse.statusText,
        headers: backendResponse.headers
      });
    } catch (error) {
      return new Response(`Proxy Hatasi: ${error.message}`, { status: 502 });
    }
  }
};