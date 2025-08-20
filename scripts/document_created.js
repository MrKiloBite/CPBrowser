(function() {
    const originalConsoleLog = console.log;
    const originalConsoleWarn = console.warn;
    const originalConsoleError = console.error;
    const originalConsoleInfo = console.info;
    const originalConsoleDebug = console.debug;

    function formatConsoleArgs(args) {
        let messageParts = [];
        for (let i = 0; i < args.length; i++) {
            let arg = args[i];
            try {
                if (arg instanceof Error) {
                    messageParts.push(arg.stack || arg.toString());
                } else if (typeof arg === 'object' && arg !== null) {
                    messageParts.push(JSON.stringify(arg, (key, value) =>
                        typeof value === 'bigint' ? value.toString() + 'n' : value 
                    ));
                } else {
                    messageParts.push(String(arg));
                }
            } catch (e) {
                messageParts.push('[Unserializable Object]');
            }
        }
        return messageParts.join(' ');
    }

    console.log = function() {
        const message = formatConsoleArgs(arguments);
        window.chrome.webview.postMessage('JS_CONSOLE_LOG::' + message);
        originalConsoleLog.apply(console, arguments);
    };
    console.warn = function() {
        const message = formatConsoleArgs(arguments);
        window.chrome.webview.postMessage('JS_CONSOLE_WARN::' + message);
        originalConsoleWarn.apply(console, arguments);
    };
    console.error = function() {
        const message = formatConsoleArgs(arguments);
        window.chrome.webview.postMessage('JS_CONSOLE_ERROR::' + message);
        originalConsoleError.apply(console, arguments);
    };
    console.info = function() {
        const message = formatConsoleArgs(arguments);
        window.chrome.webview.postMessage('JS_CONSOLE_INFO::' + message);
        originalConsoleInfo.apply(console, arguments);
    };
    console.debug = function() {
        const message = formatConsoleArgs(arguments);
        window.chrome.webview.postMessage('JS_CONSOLE_DEBUG::' + message);
        originalConsoleDebug.apply(console, arguments);
    };

    window.onerror = function(message, source, lineno, colno, error) {
        let errorMessage = 'Error: ' + message;
        if (source) errorMessage += ' Source: ' + source;
        if (lineno) errorMessage += ' Line: ' + lineno;
        if (colno) errorMessage += ' Column: ' + colno;
        if (error && error.stack) {
            errorMessage += ' Stack: ' + error.stack;
        } else if (error) {
            try { errorMessage += ' ErrorObj: ' + JSON.stringify(error); }
            catch (e) { errorMessage += ' ErrorObj: [Unserializable Error]'; }
        }
        window.chrome.webview.postMessage('JS_UNCAUGHT_ERROR::' + errorMessage);
        return false; 
    };

    window.addEventListener('unhandledrejection', function(event) {
        let reason = event.reason;
        let errorMessage = 'Unhandled Promise Rejection: ';
        if (reason instanceof Error) {
            errorMessage += (reason.stack || reason.message);
        } else {
            try { errorMessage += JSON.stringify(reason); }
            catch (e) { errorMessage += String(reason); }
        }
        window.chrome.webview.postMessage('JS_UNHANDLED_REJECTION::' + errorMessage);
    });

    document.addEventListener('mouseover', function(event) {
        let targetElement = event.target;
        while (targetElement && typeof targetElement.closest !== 'function') {
            targetElement = targetElement.parentNode;
            if (!targetElement || targetElement === document.body) break;
        }
        if (!targetElement || typeof targetElement.closest !== 'function') return;

        const anchor = targetElement.closest('a[href]');
        const button = targetElement.closest('button');
        const inputSubmit = targetElement.closest('input[type="submit"], input[type="button"]');
        const clickableRole = targetElement.closest('[role="link"], [role="button"]');

        let url = null;
        if (anchor) {
            url = anchor.href;
        } else if (targetElement.onclick || (button && button.onclick) || (inputSubmit && inputSubmit.onclick) || (clickableRole && clickableRole.onclick)) {
            url = 'javascript:... (onclick)'; 
        } else if (button || inputSubmit || clickableRole) {
            url = targetElement.tagName +
                (targetElement.id ? '#' + targetElement.id : '') +
                (targetElement.className ? '.' + targetElement.className.toString().trim().replace(/\\s+/g, '.') : '');
        }

        if (url) {
            window.chrome.webview.postMessage('HOVER_URL:' + url);
        }
    });
    document.addEventListener('mouseout', function(event) {
        window.chrome.webview.postMessage('HOVER_LEAVE');
    });
})();
