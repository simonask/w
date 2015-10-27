#ifndef W_HTTP_HPP_INCLUDED
#define W_HTTP_HPP_INCLUDED

#include <string>

#include <wayward/support/data_franca/object.hpp>
#include <wayward/support/uri.hpp>

namespace wayward {
  struct IEventLoop;

  /*
    Source:
    http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
  */
  enum class HTTPStatusCode {
    // Informational
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,

    // Success
    OK = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritative = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    IMUsed = 226,

    // Redirection
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    SwitchProxy = 306,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    // Client Error
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestURITooLong = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    AuthenticationTimeout = 419,
    MethodFailure_SpringFramework = 420,
    EnhanceYourCalm_Twitter = 420,
    UnprocessableEntity_WebDav = 422,
    Locked_WebDav = 423,
    FailedDependency_WebDav = 424,
    MethodFailure_WebDav = 424,
    UnorderedCollection = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    LoginTimeout_Microsoft = 440,
    NoResponse = 444,
    RetryWith_Microsoft = 449,
    BlockedByWindowsParentalControls_Microsoft = 450,
    UnavailableForLegalReasons_Microsoft = 451,
    Redirect_Microsoft = 451,
    RequestHeaderTooLarge_nginx = 494,
    CertError_nginx = 495,
    NoCert_nginx = 496,
    HTTPToHTTPS_nginx = 497,
    ClientClosedRequest_nginx = 499,

    // Server Error
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage_WebDav = 507,
    LoopDetected_WebDav = 508,
    BandwidthLimitExceeded_Apache = 509,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,
    OriginError_Cloudflare = 520,
    ConnectionTimedOut_Cloudflare = 522,
    ProxyDeclinedRequest_Cloudflare = 523,
    ATimeoutOccurred_Cloudflare = 524,
    NetworkReadTimeoutError_Microsoft = 598,
    NetworkConnectTimeoutError_Microsoft = 599,
  };

  using Headers = std::map<std::string, std::string>;
  using Params = data_franca::Object;

  struct Request {
    Headers headers;
    Params params;
    std::string method;
    URI uri;
    std::string body;
  };

  struct Response {
    Headers headers;
    HTTPStatusCode code = HTTPStatusCode::OK;
    std::string reason; // Keep empty to derive from `code`.
    std::string body;
  };

  struct HTTPError : Error {
    HTTPError(const std::string& msg) : Error(msg) {}
  };

  struct HTTPServer {
    HTTPServer(int socket_fd, std::function<Response(Request)> handler);
    HTTPServer(std::string listen_host, int port, std::function<Response(Request)> handler);
    ~HTTPServer();

    void start(IEventLoop* loop);
    void stop();

    struct Private;
    std::unique_ptr<Private> p_;
  };

  struct HTTPClient {
    explicit HTTPClient(std::string host, int port = 80);
    ~HTTPClient();

    std::string host() const;
    int port() const;

    using Headers = std::map<std::string, std::string>;

    Response request(Request req);
    Response request(std::string method, std::string path, Maybe<std::string> body, Params = Params{}, Headers = Headers{});

    Response get(std::string path, Params = Params{}, Headers = Headers{});
    Response post(std::string path, Params = Params{}, Headers = Headers{});
    Response post(std::string path, std::string body, Params = Params{}, Headers = Headers{});
    Response put(std::string path, Params = Params{}, Headers = Headers{});
    Response put(std::string path, std::string body, Params = Params{}, Headers = Headers{});
    Response patch(std::string path, Params = Params{}, Headers = Headers{});
    Response patch(std::string path, std::string body, Params = Params{}, Headers = Headers{});
    Response del(std::string path, Params = Params{}, Headers = Headers{});
    Response options(std::string path, Params = Params{}, Headers = Headers{});
    Response head(std::string path, Params = Params{}, Headers = Headers{});

    struct Private;
    std::unique_ptr<Private> p_;
  };
}

#endif /* end of include guard: SYMBOL */
