try:
    import uasyncio as asyncio
except ImportError:
    import asyncio
import io
try:
    import ujson as json
except ImportError:
    import json

MUTED_SOCKET_ERRORS = [32, 54, 104, 128]

class NoCaseDict(dict):
    def __init__(self, initial_dict=None):
        super().__init__(initial_dict or {})
        self.keymap = {k.lower(): k for k in self.keys() if k.lower() != k}
    def __setitem__(self, key, value):
        kl = key.lower()
        key = self.keymap.get(kl, key)
        if kl != key: self.keymap[kl] = key
        super().__setitem__(key, value)
    def __getitem__(self, key):
        kl = key.lower()
        return super().__getitem__(self.keymap.get(kl, kl))
    def get(self, key, default=None):
        kl = key.lower()
        return super().get(self.keymap.get(kl, kl), default)

class MultiDict(dict):
    def __init__(self, initial_dict=None):
        super().__init__()
        if initial_dict:
            for key, value in initial_dict.items():
                self[key] = value
    def __setitem__(self, key, value):
        if key not in self:
            super().__setitem__(key, [])
        super().__getitem__(key).append(value)
    def __getitem__(self, key):
        return super().__getitem__(key)[0]
    def get(self, key, default=None):
        if key not in self:
            return default
        return self[key]

class AsyncBytesIO:
    def __init__(self, data):
        self.stream = io.BytesIO(data)
    async def read(self, n=-1):
        return self.stream.read(n)
    async def awrite(self, data):
        return self.stream.write(data)
    async def aclose(self):
        pass

class Request:
    max_content_length = 16 * 1024
    max_body_length = 16 * 1024
    max_readline = 2 * 1024
    class G: pass
    def __init__(self, app, client_addr, method, url, http_version, headers, body=None, stream=None):
        self.app = app
        self.client_addr = client_addr
        self.method = method
        self.url = url
        self.path = url
        self.headers = headers
        self.content_length = 0
        self.content_type = None
        self.g = Request.G()
        if '?' in self.path:
            self.path, _ = self.path.split('?', 1)
        if 'Content-Length' in self.headers:
            self.content_length = int(self.headers['Content-Length'])
        if 'Content-Type' in self.headers:
            self.content_type = self.headers['Content-Type']
        self._body = body
        self._stream = stream
        self._json = None
        self._form = None
    @staticmethod
    async def create(app, client_reader, client_writer, client_addr):
        line = (await Request._safe_readline(client_reader)).strip().decode()
        if not line: return None
        method, url, http_version = line.split()
        http_version = http_version.split('/', 1)[1]
        headers = NoCaseDict()
        content_length = 0
        while True:
            line = (await Request._safe_readline(client_reader)).strip().decode()
            if line == '': break
            header, value = line.split(':', 1)
            headers[header] = value.strip()
            if header.lower() == 'content-length':
                content_length = int(value)
        body = b''
        if content_length and content_length <= Request.max_body_length:
            body = await client_reader.readexactly(content_length)
            stream = None
        else:
            body = b''
            stream = client_reader
        return Request(app, client_addr, method, url, http_version, headers, body=body, stream=stream)
    @property
    def body(self): return self._body
    @property
    def stream(self):
        if self._stream is None:
            self._stream = AsyncBytesIO(self._body)
        return self._stream
    @property
    def json(self):
        if self._json is None:
            if self.content_type is None: return None
            mime_type = self.content_type.split(';')[0]
            if mime_type != 'application/json': return None
            self._json = json.loads(self.body.decode())
        return self._json
    @property
    def form(self):
        if self._form is None:
            if self.content_type is None: return None
            mime_type = self.content_type.split(';')[0]
            if mime_type != 'application/x-www-form-urlencoded': return None
            from urllib.parse import parse_qs
            self._form = MultiDict()
            for k, v in parse_qs(self.body.decode()).items():
                self._form[k] = v[0]
        return self._form
    @staticmethod
    async def _safe_readline(stream):
        line = (await stream.readline())
        if len(line) > Request.max_readline:
            raise ValueError('line too long')
        return line

class Response:
    default_content_type = 'text/plain'
    def __init__(self, body='', status_code=200, headers=None):
        self.status_code = status_code
        self.headers = NoCaseDict(headers or {})
        if isinstance(body, (dict, list)):
            body = json.dumps(body)
            self.headers['Content-Type'] = 'application/json; charset=UTF-8'
        self.body = body.encode() if isinstance(body, str) else body
    def complete(self):
        if isinstance(self.body, bytes) and 'Content-Length' not in self.headers:
            self.headers['Content-Length'] = str(len(self.body))
        if 'Content-Type' not in self.headers:
            self.headers['Content-Type'] = self.default_content_type + '; charset=UTF-8'
    async def write(self, stream):
        self.complete()
        try:
            reason = 'OK' if self.status_code == 200 else 'N/A'
            await stream.awrite(f'HTTP/1.0 {self.status_code} {reason}\r\n'.encode())
            for header, value in self.headers.items():
                await stream.awrite(f'{header}: {value}\r\n'.encode())
            await stream.awrite(b'\r\n')
            await stream.awrite(self.body if isinstance(self.body, bytes) else self.body.encode())
        except OSError as exc:
            if exc.errno not in MUTED_SOCKET_ERRORS:
                raise

class Microdot:
    def __init__(self):
        self.url_map = []
    def route(self, url_pattern, methods=None):
        def decorated(f):
            self.url_map.append(([m.upper() for m in (methods or ['GET'])], url_pattern, f))
            return f
        return decorated
    def get(self, url_pattern): return self.route(url_pattern, ['GET'])
    def post(self, url_pattern): return self.route(url_pattern, ['POST'])
    async def handle_request(self, reader, writer):
        req = None
        try:
            req = await Request.create(self, reader, writer, writer.get_extra_info('peername'))
        except OSError as exc:
            if exc.errno in MUTED_SOCKET_ERRORS: return
            raise
        except Exception: return
        res = await self.dispatch_request(req)
        try:
            await res.write(writer)
            await writer.aclose()
        except OSError as exc:
            if exc.errno in MUTED_SOCKET_ERRORS: pass
            else: raise
    async def dispatch_request(self, req):
        if not req: return Response('Bad request', 400)
        if req.content_length > req.max_content_length:
            return Response('Payload too large', 413)
        handler = None
        for methods, pattern, h in self.url_map:
            if req.path == pattern and req.method in methods:
                handler = h
                break
        if not handler: return Response('Not found', 404)
        try:
            res = handler(req)
            if isinstance(res, tuple):
                body, status, *extra = res
                headers = extra[0] if extra else {}
                res = Response(body, status, headers)
            elif not isinstance(res, Response):
                res = Response(res)
            return res
        except Exception as e:
            return Response(f'Error: {e}', 500)
    def run(self, host='0.0.0.0', port=80):
        print(f'Microdot on http://{host}:{port}')
        loop = asyncio.get_event_loop()
        loop.create_task(asyncio.start_server(self.handle_request, host, port))
        loop.run_forever()

abort = lambda status, reason=None: (_ for _ in ()).throw(Exception(f'HTTP {status}'))
redirect = lambda url, code=302: Response('', code, {'Location': url})