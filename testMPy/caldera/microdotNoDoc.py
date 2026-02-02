try:
    import uasyncio as asyncio
except ImportError:
    import asyncio
import io
import re
import time

try:
    import orjson as json
except ImportError:
    import json

try:
    from inspect import iscoroutinefunction, iscoroutine
    from functools import partial
    async def invoke_handler(handler, *args, **kwargs):
        if iscoroutinefunction(handler):
            ret = await handler(*args, **kwargs)
        else:
            ret = await asyncio.get_running_loop().run_in_executor(None, partial(handler, *args, **kwargs))
        return ret
except ImportError:
    def iscoroutine(coro):
        return hasattr(coro, 'send') and hasattr(coro, 'throw')
    async def invoke_handler(handler, *args, **kwargs):
        ret = handler(*args, **kwargs)
        if iscoroutine(ret):
            ret = await ret
        return ret

try:
    from sys import print_exception
except ImportError:
    import traceback
    def print_exception(exc):
        traceback.print_exc()

MUTED_SOCKET_ERRORS = [32, 54, 104, 128]

def urldecode(s):
    if isinstance(s, str):
        s = s.encode()
    s = s.replace(b'+', b' ')
    parts = s.split(b'%')
    if len(parts) == 1:
        return s.decode()
    result = [parts[0]]
    for item in parts[1:]:
        if item == b'':
            result.append(b'%')
        else:
            code = item[:2]
            result.append(bytes([int(code, 16)]))
            result.append(item[2:])
    return b''.join(result).decode()

def urlencode(s):
    return s.replace('+', '%2B').replace(' ', '+').replace('%', '%25').replace('?', '%3F').replace('#', '%23').replace('&', '%26').replace('=', '%3D')

class NoCaseDict(dict):
    def __init__(self, initial_dict=None):
        super().__init__(initial_dict or {})
        self.keymap = {k.lower(): k for k in self.keys() if k.lower() != k}
    def __setitem__(self, key, value):
        kl = key.lower()
        key = self.keymap.get(kl, key)
        if kl != key:
            self.keymap[kl] = key
        super().__setitem__(key, value)
    def __getitem__(self, key):
        kl = key.lower()
        return super().__getitem__(self.keymap.get(kl, kl))
    def __delitem__(self, key):
        kl = key.lower()
        super().__delitem__(self.keymap.get(kl, kl))
    def __contains__(self, key):
        kl = key.lower()
        return self.keymap.get(kl, kl) in self.keys()
    def get(self, key, default=None):
        kl = key.lower()
        return super().get(self.keymap.get(kl, kl), default)
    def update(self, other_dict):
        for key, value in other_dict.items():
            self[key] = value

def mro(cls):
    if hasattr(cls, 'mro'):
        return cls.__mro__
    def _mro(cls):
        m = [cls]
        for base in cls.__bases__:
            m += _mro(base)
        return m
    mro_list = _mro(cls)
    mro_pruned = []
    for i in range(len(mro_list)):
        base = mro_list.pop(0)
        if base not in mro_list:
            mro_pruned.append(base)
    return mro_pruned

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
    def get(self, key, default=None, type=None):
        if key not in self:
            return default
        value = self[key]
        if type is not None:
            value = type(value)
        return value
    def getlist(self, key, type=None):
        if key not in self:
            return []
        values = super().__getitem__(key)
        if type is not None:
            values = [type(value) for value in values]
        return values

class AsyncBytesIO:
    def __init__(self, data):
        self.stream = io.BytesIO(data)
    async def read(self, n=-1):
        return self.stream.read(n)
    async def readline(self):
        return self.stream.readline()
    async def readexactly(self, n):
        return self.stream.read(n)
    async def readuntil(self, separator=b'\n'):
        return self.stream.readuntil(separator=separator)
    async def awrite(self, data):
        return self.stream.write(data)
    async def aclose(self):
        pass

class Request:
    max_content_length = 16 * 1024
    max_body_length = 16 * 1024
    max_readline = 2 * 1024
    class G:
        pass
    def __init__(self, app, client_addr, method, url, http_version, headers, body=None, stream=None, sock=None, url_prefix='', subapp=None, scheme=None, route=None):
        self.app = app
        self.client_addr = client_addr
        self.method = method
        self.scheme = scheme or 'http'
        self.url = url
        self.url_prefix = url_prefix
        self.subapp = subapp
        self.route = route
        self.path = url
        self.query_string = None
        self.args = {}
        self.headers = headers
        self.cookies = {}
        self.content_length = 0
        self.content_type = None
        self.g = Request.G()
        self.http_version = http_version
        if '?' in self.path:
            self.path, self.query_string = self.path.split('?', 1)
            self.args = self._parse_urlencoded(self.query_string)
        if 'Content-Length' in self.headers:
            self.content_length = int(self.headers['Content-Length'])
        if 'Content-Type' in self.headers:
            self.content_type = self.headers['Content-Type']
        if 'Cookie' in self.headers:
            for cookie in self.headers['Cookie'].split(';'):
                c = cookie.strip().split('=', 1)
                self.cookies[c[0]] = c[1] if len(c) > 1 else ''
        self._body = body
        self.body_used = False
        self._stream = stream
        self.sock = sock
        self._json = None
        self._form = None
        self._files = None
        self.after_request_handlers = []
    @staticmethod
    async def create(app, client_reader, client_writer, client_addr, scheme=None):
        line = (await Request._safe_readline(client_reader)).strip().decode()
        if not line:
            return None
        method, url, http_version = line.split()
        http_version = http_version.split('/', 1)[1]
        headers = NoCaseDict()
        content_length = 0
        while True:
            line = (await Request._safe_readline(client_reader)).strip().decode()
            if line == '':
                break
            header, value = line.split(':', 1)
            value = value.strip()
            headers[header] = value
            if header.lower() == 'content-length':
                content_length = int(value)
        body = b''
        if content_length and content_length <= Request.max_body_length:
            body = await client_reader.readexactly(content_length)
            stream = None
        else:
            body = b''
            stream = client_reader
        return Request(app, client_addr, method, url, http_version, headers, body=body, stream=stream, sock=(client_reader, client_writer), scheme=scheme)
    def _parse_urlencoded(self, urlencoded):
        data = MultiDict()
        if len(urlencoded) > 0:
            if isinstance(urlencoded, str):
                for kv in [pair.split('=', 1) for pair in urlencoded.split('&') if pair]:
                    data[urldecode(kv[0])] = urldecode(kv[1]) if len(kv) > 1 else ''
            elif isinstance(urlencoded, bytes):
                for kv in [pair.split(b'=', 1) for pair in urlencoded.split(b'&') if pair]:
                    data[urldecode(kv[0])] = urldecode(kv[1]) if len(kv) > 1 else b''
        return data
    @property
    def body(self):
        return self._body
    @property
    def stream(self):
        if self._stream is None:
            self._stream = AsyncBytesIO(self._body)
        return self._stream
    @property
    def json(self):
        if self._json is None:
            if self.content_type is None:
                return None
            mime_type = self.content_type.split(';')[0]
            if mime_type != 'application/json':
                return None
            self._json = json.loads(self.body.decode())
        return self._json
    @property
    def form(self):
        if self._form is None:
            if self.content_type is None:
                return None
            mime_type = self.content_type.split(';')[0]
            if mime_type != 'application/x-www-form-urlencoded':
                return None
            self._form = self._parse_urlencoded(self.body)
        return self._form
    @property
    def files(self):
        return self._files
    def after_request(self, f):
        self.after_request_handlers.append(f)
        return f
    @staticmethod
    async def _safe_readline(stream):
        line = (await stream.readline())
        if len(line) > Request.max_readline:
            raise ValueError('line too long')
        return line

class Response:
    types_map = {'css': 'text/css', 'gif': 'image/gif', 'html': 'text/html', 'jpg': 'image/jpeg', 'js': 'application/javascript', 'json': 'application/json', 'png': 'image/png', 'txt': 'text/plain', 'svg': 'image/svg+xml'}
    send_file_buffer_size = 1024
    default_content_type = 'text/plain'
    default_send_file_max_age = None
    already_handled = None
    def __init__(self, body='', status_code=200, headers=None, reason=None):
        if body is None and status_code == 200:
            body = ''
            status_code = 204
        self.status_code = status_code
        self.headers = NoCaseDict(headers or {})
        self.reason = reason
        if isinstance(body, (dict, list)):
            body = json.dumps(body)
            self.headers['Content-Type'] = 'application/json; charset=UTF-8'
        if isinstance(body, str):
            self.body = body.encode()
        else:
            self.body = body
        self.is_head = False
    def set_cookie(self, cookie, value, path=None, domain=None, expires=None, max_age=None, secure=False, http_only=False, partitioned=False):
        http_cookie = '{cookie}={value}'.format(cookie=cookie, value=value)
        if path:
            http_cookie += '; Path=' + path
        if domain:
            http_cookie += '; Domain=' + domain
        if expires:
            if isinstance(expires, str):
                http_cookie += '; Expires=' + expires
            else:
                http_cookie += '; Expires=' + time.strftime('%a, %d %b %Y %H:%M:%S GMT', expires.timetuple())
        if max_age is not None:
            http_cookie += '; Max-Age=' + str(max_age)
        if secure:
            http_cookie += '; Secure'
        if http_only:
            http_cookie += '; HttpOnly'
        if partitioned:
            http_cookie += '; Partitioned'
        if 'Set-Cookie' in self.headers:
            self.headers['Set-Cookie'].append(http_cookie)
        else:
            self.headers['Set-Cookie'] = [http_cookie]
    def delete_cookie(self, cookie, **kwargs):
        kwargs.pop('expires', None)
        kwargs.pop('max_age', None)
        self.set_cookie(cookie, '', expires='Thu, 01 Jan 1970 00:00:01 GMT', max_age=0, **kwargs)
    def complete(self):
        if isinstance(self.body, bytes) and 'Content-Length' not in self.headers:
            self.headers['Content-Length'] = str(len(self.body))
        if 'Content-Type' not in self.headers:
            self.headers['Content-Type'] = self.default_content_type
            if 'charset=' not in self.headers['Content-Type']:
                self.headers['Content-Type'] += '; charset=UTF-8'
    async def write(self, stream):
        self.complete()
        try:
            reason = self.reason if self.reason is not None else ('OK' if self.status_code == 200 else 'N/A')
            await stream.awrite('HTTP/1.0 {status_code} {reason}\r\n'.format(status_code=self.status_code, reason=reason).encode())
            for header, value in self.headers.items():
                values = value if isinstance(value, list) else [value]
                for value in values:
                    await stream.awrite('{header}: {value}\r\n'.format(header=header, value=value).encode())
            await stream.awrite(b'\r\n')
            if not self.is_head:
                iter = self.body_iter()
                async for body in iter:
                    if isinstance(body, str):
                        body = body.encode()
                    try:
                        await stream.awrite(body)
                    except OSError as exc:
                        if exc.errno in MUTED_SOCKET_ERRORS or exc.args[0] == 'Connection lost':
                            if hasattr(iter, 'aclose'):
                                await iter.aclose()
                        raise
                if hasattr(iter, 'aclose'):
                    await iter.aclose()
        except OSError as exc:
            if exc.errno in MUTED_SOCKET_ERRORS or exc.args[0] == 'Connection lost':
                pass
            else:
                raise
    def body_iter(self):
        if hasattr(self.body, '__anext__'):
            return self.body
        response = self
        class iter:
            ITER_UNKNOWN = 0
            ITER_SYNC_GEN = 1
            ITER_FILE_OBJ = 2
            ITER_NO_BODY = -1
            def __aiter__(self):
                if response.body:
                    self.i = self.ITER_UNKNOWN
                else:
                    self.i = self.ITER_NO_BODY
                return self
            async def __anext__(self):
                if self.i == self.ITER_NO_BODY:
                    await self.aclose()
                    raise StopAsyncIteration
                if self.i == self.ITER_UNKNOWN:
                    if hasattr(response.body, 'read'):
                        self.i = self.ITER_FILE_OBJ
                    elif hasattr(response.body, '__next__'):
                        self.i = self.ITER_SYNC_GEN
                        return next(response.body)
                    else:
                        self.i = self.ITER_NO_BODY
                        return response.body
                elif self.i == self.ITER_SYNC_GEN:
                    try:
                        return next(response.body)
                    except StopIteration:
                        await self.aclose()
                        raise StopAsyncIteration
                buf = response.body.read(response.send_file_buffer_size)
                if iscoroutine(buf):
                    buf = await buf
                if len(buf) < response.send_file_buffer_size:
                    self.i = self.ITER_NO_BODY
                return buf
            async def aclose(self):
                if hasattr(response.body, 'close'):
                    result = response.body.close()
                    if iscoroutine(result):
                        await result
        return iter()
    @classmethod
    def redirect(cls, location, status_code=302):
        if '\x0d' in location or '\x0a' in location:
            raise ValueError('invalid redirect URL')
        return cls(status_code=status_code, headers={'Location': location})
    @classmethod
    def send_file(cls, filename, status_code=200, content_type=None, stream=None, max_age=None, compressed=False, file_extension=''):
        if content_type is None:
            if compressed and filename.endswith('.gz'):
                ext = filename[:-3].split('.')[-1]
            else:
                ext = filename.split('.')[-1]
            if ext in Response.types_map:
                content_type = Response.types_map[ext]
            else:
                content_type = 'application/octet-stream'
        headers = {'Content-Type': content_type}
        if max_age is None:
            max_age = cls.default_send_file_max_age
        if max_age is not None:
            headers['Cache-Control'] = 'max-age={}'.format(max_age)
        if compressed:
            headers['Content-Encoding'] = compressed if isinstance(compressed, str) else 'gzip'
        f = stream or open(filename + file_extension, 'rb')
        return cls(body=f, status_code=status_code, headers=headers)

class URLPattern:
    segment_patterns = {'string': '/([^/]+)', 'int': '/(-?\\d+)', 'path': '/(.+)'}
    segment_parsers = {'int': lambda value: int(value)}
    @classmethod
    def register_type(cls, type_name, pattern='[^/]+', parser=None):
        cls.segment_patterns[type_name] = '/({})'.format(pattern)
        cls.segment_parsers[type_name] = parser
    def __init__(self, url_pattern):
        self.url_pattern = url_pattern
        self.segments = []
        self.regex = None
    def compile(self):
        pattern = ''
        for segment in self.url_pattern.lstrip('/').split('/'):
            if segment and segment[0] == '<':
                if segment[-1] != '>':
                    raise ValueError('invalid URL pattern')
                segment = segment[1:-1]
                if ':' in segment:
                    type_, name = segment.rsplit(':', 1)
                else:
                    type_ = 'string'
                    name = segment
                parser = None
                if type_.startswith('re:'):
                    pattern += '/({pattern})'.format(pattern=type_[3:])
                else:
                    if type_ not in self.segment_patterns:
                        raise ValueError('invalid URL segment type')
                    pattern += self.segment_patterns[type_]
                    parser = self.segment_parsers.get(type_)
                self.segments.append({'parser': parser, 'name': name, 'type': type_})
            else:
                pattern += '/' + segment
                self.segments.append({'parser': None})
        self.regex = re.compile('^' + pattern + '$')
        return self.regex
    def match(self, path):
        args = {}
        g = (self.regex or self.compile()).match(path)
        if not g:
            return
        i = 1
        for segment in self.segments:
            if 'name' not in segment:
                continue
            arg = g.group(i)
            if segment['parser']:
                arg = self.segment_parsers[segment['type']](arg)
                if arg is None:
                    return
            args[segment['name']] = arg
            i += 1
        return args

class HTTPException(Exception):
    def __init__(self, status_code, reason=None):
        self.status_code = status_code
        self.reason = reason or str(status_code) + ' error'

class Microdot:
    def __init__(self):
        self.url_map = []
        self.before_request_handlers = []
        self.after_request_handlers = []
        self.after_error_request_handlers = []
        self.error_handlers = {}
        self.options_handler = self.default_options_handler
        self.ssl = False
        self.debug = False
        self.server = None
    def route(self, url_pattern, methods=None):
        def decorated(f):
            self.url_map.append(([m.upper() for m in (methods or ['GET'])], URLPattern(url_pattern), f, '', None))
            return f
        return decorated
    def get(self, url_pattern):
        return self.route(url_pattern, methods=['GET'])
    def post(self, url_pattern):
        return self.route(url_pattern, methods=['POST'])
    def put(self, url_pattern):
        return self.route(url_pattern, methods=['PUT'])
    def patch(self, url_pattern):
        return self.route(url_pattern, methods=['PATCH'])
    def delete(self, url_pattern):
        return self.route(url_pattern, methods=['DELETE'])
    def before_request(self, f):
        self.before_request_handlers.append(f)
        return f
    def after_request(self, f):
        self.after_request_handlers.append(f)
        return f
    def after_error_request(self, f):
        self.after_error_request_handlers.append(f)
        return f
    def errorhandler(self, status_code_or_exception_class):
        def decorated(f):
            self.error_handlers[status_code_or_exception_class] = f
            return f
        return decorated
    def mount(self, subapp, url_prefix='', local=False):
        for methods, pattern, handler, _prefix, _subapp in subapp.url_map:
            self.url_map.append((methods, URLPattern(url_prefix + pattern.url_pattern), handler, url_prefix + _prefix, _subapp or subapp))
        if not local:
            for handler in subapp.before_request_handlers:
                self.before_request_handlers.append(handler)
            subapp.before_request_handlers = []
            for handler in subapp.after_request_handlers:
                self.after_request_handlers.append(handler)
            subapp.after_request_handlers = []
            for handler in subapp.after_error_request_handlers:
                self.after_error_request_handlers.append(handler)
            subapp.after_error_request_handlers = []
            for status_code, handler in subapp.error_handlers.items():
                self.error_handlers[status_code] = handler
            subapp.error_handlers = {}
    @staticmethod
    def abort(status_code, reason=None):
        raise HTTPException(status_code, reason)
    async def start_server(self, host='0.0.0.0', port=5000, debug=False, ssl=None):
        self.ssl = ssl
        self.debug = debug
        async def serve(reader, writer):
            if not hasattr(writer, 'awrite'):
                async def awrite(self, data):
                    self.write(data)
                    await self.drain()
                async def aclose(self):
                    self.close()
                    await self.wait_closed()
                from types import MethodType
                writer.awrite = MethodType(awrite, writer)
                writer.aclose = MethodType(aclose, writer)
            await self.handle_request(reader, writer)
        if self.debug:
            print('Starting async server on {host}:{port}...'.format(host=host, port=port))
        try:
            self.server = await asyncio.start_server(serve, host, port, ssl=ssl)
        except TypeError:
            self.server = await asyncio.start_server(serve, host, port)
        while True:
            try:
                if hasattr(self.server, 'serve_forever'):
                    try:
                        await self.server.serve_forever()
                    except asyncio.CancelledError:
                        pass
                await self.server.wait_closed()
                break
            except AttributeError:
                await asyncio.sleep(0.1)
    def run(self, host='0.0.0.0', port=5000, debug=False, ssl=None):
        asyncio.run(self.start_server(host=host, port=port, debug=debug, ssl=ssl))
    def shutdown(self):
        self.server.close()
    def find_route(self, req):
        method = req.method.upper()
        if method == 'OPTIONS' and self.options_handler:
            return self.options_handler(req), '', None
        if method == 'HEAD':
            method = 'GET'
        f = 404
        p = ''
        s = None
        for route_methods, route_pattern, route_handler, url_prefix, subapp in self.url_map:
            req.url_args = route_pattern.match(req.path)
            if req.url_args is not None:
                p = url_prefix
                s = subapp
                if method in route_methods:
                    f = route_handler
                    break
                else:
                    f = 405
        return f, p, s
    def default_options_handler(self, req):
        allow = []
        for route_methods, route_pattern, _, _, _ in self.url_map:
            if route_pattern.match(req.path) is not None:
                allow.extend(route_methods)
        if 'GET' in allow:
            allow.append('HEAD')
        allow.append('OPTIONS')
        return {'Allow': ', '.join(allow)}
    async def handle_request(self, reader, writer):
        req = None
        try:
            req = await Request.create(self, reader, writer, writer.get_extra_info('peername'))
        except OSError as exc:
            if exc.errno in MUTED_SOCKET_ERRORS:
                pass
            else:
                raise
        except Exception as exc:
            print_exception(exc)
        res = await self.dispatch_request(req)
        try:
            if res != Response.already_handled:
                await res.write(writer)
            await writer.aclose()
        except OSError as exc:
            if exc.errno in MUTED_SOCKET_ERRORS:
                pass
            else:
                raise
        if self.debug and req:
            print('{method} {path} {status_code}'.format(method=req.method, path=req.path, status_code=res.status_code))
    def get_request_handlers(self, req, attr, local_first=True):
        handlers = getattr(self, attr + '_handlers')
        local_handlers = getattr(req.subapp, attr + '_handlers') if req and req.subapp else []
        return local_handlers + handlers if local_first else handlers + local_handlers
    async def error_response(self, req, status_code, reason=None):
        if req and req.subapp and status_code in req.subapp.error_handlers:
            return await invoke_handler(req.subapp.error_handlers[status_code], req)
        elif status_code in self.error_handlers:
            return await invoke_handler(self.error_handlers[status_code], req)
        return reason or 'N/A', status_code
    async def dispatch_request(self, req):
        after_request_handled = False
        if req:
            if req.content_length > req.max_content_length:
                res = await self.error_response(req, 413, 'Payload too large')
            else:
                f, req.url_prefix, req.subapp = self.find_route(req)
                try:
                    res = None
                    if callable(f):
                        req.route = f
                        for handler in self.get_request_handlers(req, 'before_request', False):
                            res = await invoke_handler(handler, req)
                            if res:
                                break
                        if res is None:
                            res = await invoke_handler(f, req, **req.url_args)
                        if isinstance(res, int):
                            res = '', res
                        if isinstance(res, tuple):
                            if isinstance(res[0], int):
                                res = ('', res[0], res[1] if len(res) > 1 else {})
                            body = res[0]
                            if isinstance(res[1], int):
                                status_code = res[1]
                                headers = res[2] if len(res) > 2 else {}
                            else:
                                status_code = 200
                                headers = res[1]
                            res = Response(body, status_code, headers)
                        elif not isinstance(res, Response):
                            res = Response(res)
                        for handler in self.get_request_handlers(req, 'after_request', True):
                            res = await invoke_handler(handler, req, res) or res
                        for handler in req.after_request_handlers:
                            res = await invoke_handler(handler, req, res) or res
                        after_request_handled = True
                    elif isinstance(f, dict):
                        res = Response(headers=f)
                    else:
                        res = await self.error_response(req, f, 'Not found')
                except HTTPException as exc:
                    res = await self.error_response(req, exc.status_code, exc.reason)
                except Exception as exc:
                    print_exception(exc)
                    handler = None
                    res = None
                    if req.subapp and exc.__class__ in req.subapp.error_handlers:
                        handler = req.subapp.error_handlers[exc.__class__]
                    elif exc.__class__ in self.error_handlers:
                        handler = self.error_handlers[exc.__class__]
                    else:
                        for c in mro(exc.__class__)[1:]:
                            if req.subapp and c in req.subapp.error_handlers:
                                handler = req.subapp.error_handlers[c]
                                break
                            elif c in self.error_handlers:
                                handler = self.error_handlers[c]
                                break
                    if handler:
                        try:
                            res = await invoke_handler(handler, req, exc)
                        except Exception as exc2:
                            print_exception(exc2)
                    if res is None:
                        res = await self.error_response(req, 500, 'Internal server error')
        else:
            res = await self.error_response(req, 400, 'Bad request')
        if isinstance(res, tuple):
            res = Response(*res)
        elif not isinstance(res, Response):
            res = Response(res)
        if not after_request_handled:
            for handler in self.get_request_handlers(req, 'after_error_request', True):
                res = await invoke_handler(handler, req, res) or res
        res.is_head = (req and req.method == 'HEAD')
        return res

Response.already_handled = Response()
abort = Microdot.abort
redirect = Response.redirect
send_file = Response.send_file