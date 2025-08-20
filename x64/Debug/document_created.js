(function() {
    // --- Store original console functions to avoid loops and maintain functionality ---
    const originalConsoleLog = console.log;
    const originalConsoleWarn = console.warn;
    const originalConsoleError = console.error;
    const originalConsoleInfo = console.info;
    const originalConsoleDebug = console.debug;

    // --- Helper function to format any argument type into a string for logging ---
    function formatConsoleArgs(args) {
        let messageParts = [];
        for (let i = 0; i < args.length; i++) {
            let arg = args[i];
            try {
                if (arg instanceof Error) {
                    messageParts.push(arg.stack || arg.toString());
                } else if (typeof arg === 'object' && arg !== null) {
                    // Safely stringify objects, handling potential circular references
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
    
    // --- NEW: Function to send messages back to the C++ host ---
    function postHostMessage(prefix, message) {
        try {
            window.chrome.webview.postMessage(prefix + '::' + message);
        } catch (e) {
            // This might fail if the webview is being torn down, so we catch the error silently.
        }
    }

    // --- Hijack console functions to intercept messages ---
    console.log = function() {
        postHostMessage('JS_CONSOLE_LOG', formatConsoleArgs(arguments));
        originalConsoleLog.apply(console, arguments);
    };
    console.warn = function() {
        postHostMessage('JS_CONSOLE_WARN', formatConsoleArgs(arguments));
        originalConsoleWarn.apply(console, arguments);
    };
    console.error = function() {
        postHostMessage('JS_CONSOLE_ERROR', formatConsoleArgs(arguments));
        originalConsoleError.apply(console, arguments);
    };
    console.info = function() {
        postHostMessage('JS_CONSOLE_INFO', formatConsoleArgs(arguments));
        originalConsoleInfo.apply(console, arguments);
    };
    console.debug = function() {
        postHostMessage('JS_CONSOLE_DEBUG', formatConsoleArgs(arguments));
        originalConsoleDebug.apply(console, arguments);
    };

    // --- Hijack global error handlers ---
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
        postHostMessage('JS_UNCAUGHT_ERROR', errorMessage);
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
        postHostMessage('JS_UNHANDLED_REJECTION', errorMessage);
    });

    // --- Hover event listeners for link inspection ---
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
                (targetElement.className ? '.' + targetElement.className.toString().trim().replace(/\s+/g, '.') : '');
        }

        if (url) {
            postHostMessage('HOVER_URL', url);
        }
    });
    document.addEventListener('mouseout', function(event) {
        postHostMessage('HOVER_LEAVE', ''); // Send empty message
    });
    
    // --- NEW: Proactive analysis run immediately on injection ---
    function performInitialAnalysis() {
        postHostMessage('JS_INJECTION_INFO', '--- JS Capture Script Injected and Initialized ---');
        
        // Report User Agent as seen by JavaScript
        postHostMessage('JS_INJECTION_INFO', 'Navigator User Agent: ' + navigator.userAgent);
        
        // Report number of script tags
        const headScripts = document.head.getElementsByTagName('script').length;
        const bodyScripts = document.body.getElementsByTagName('script').length;
        postHostMessage('JS_INJECTION_INFO', 'Script Tags Found: ' + headScripts + ' in <head>, ' + bodyScripts + ' in <body>.');
        
        // Check for common libraries
        let tech = [];
        if (typeof jQuery !== 'undefined') tech.push('jQuery v' + (jQuery.fn.jquery || 'Unknown'));
        if (typeof React !== 'undefined') tech.push('React v' + (React.version || 'Unknown'));
        if (typeof Vue !== 'undefined') tech.push('Vue v' + (Vue.version || 'Unknown'));
        if (typeof angular !== 'undefined') tech.push('Angular v' + (angular.version ? angular.version.full : 'Unknown'));

        if (tech.length > 0) {
            postHostMessage('JS_INJECTION_INFO', 'Detected Libraries: ' + tech.join(', '));
        } else {
            postHostMessage('JS_INJECTION_INFO', 'No common JS libraries (jQuery, React, Vue, Angular) detected in global scope.');
        }
    }
    
    // The script is injected at document_start, but the body may not be available.
    // We wait for the DOMContentLoaded event to ensure we can inspect the full DOM.
    window.addEventListener('DOMContentLoaded', (event) => {
        performInitialAnalysis();
    });

})();
