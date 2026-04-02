"use strict";

var utils = window.utils || {};

utils.enableForm = function(formId, enableIt) {
    var form        = document.getElementById(formId);
    var elements    = form.elements;
    var index       = 0;

    for (index = 0; index < elements.length; ++index) {
        elements[index].disabled = (false === enableIt) ? true : false;
    }
};

utils.injectOrigin = function(name, searchFor) {
    var elements = document.getElementsByName(name);
    var index   = 0;

    for(index = 0; index < elements.length; ++index) {
        elements[index].innerHTML = elements[index].innerHTML.replace(searchFor, location.origin);
    }
};

utils.obj2FormData = function(obj, formData = new FormData()) {

    this.formData = formData;

    this.createFormData = function(obj, subKeyStr = "") {
        
        for(let i in obj) {
            let value          = obj[i];
            let subKeyStrTrans;
            
            if (obj instanceof Array) {
                subKeyStrTrans = subKeyStr ? subKeyStr + "._" + i + "_" : i;
            } else {
                subKeyStrTrans = subKeyStr ? subKeyStr + "." + i : i;
            }                

            if ((typeof(value) === "string") || (typeof(value) === "number")|| (typeof(value) === "boolean") || (value instanceof File)) {

                this.formData.append(subKeyStrTrans, value);

            } else if (typeof(value) === "object") {

                this.createFormData(value, subKeyStrTrans);
            }
        }
    }

    this.createFormData(obj);

    return this.formData;
};

utils.makeRequest = function(options) {
    return new Promise(function(resolve, reject) {
        if ("object" !== typeof options) {
            reject({ msg: "Arguments are missing." });
        } else if ("string" !== typeof options.method) {
            reject({ msg: "Request method is missing." });
        } else if ("string" !== typeof options.url) {
            reject({ msg: "URL is missing." });
        } else {
            var xhr             = new XMLHttpRequest();
            var formData        = null;
            var urlEncodedPar   = "";
            var isJsonResponse  = false;
            var isFirst         = true;
            var key;

            if ("object" === typeof options.formData) {
                formData = options.formData;
            }
            else if ("object" === typeof options.parameter) {
                if ("get" === options.method.toLowerCase()) {
                    urlEncodedPar += "?";

                    for(key in options.parameter) {
                        if (true === isFirst) {
                            isFirst = false;
                        } else {
                            urlEncodedPar += "&";
                        }
                        urlEncodedPar += encodeURIComponent(key);
                        urlEncodedPar += "=";
                        urlEncodedPar += encodeURIComponent(options.parameter[key]);
                    }
                } else {
                    formData = utils.obj2FormData(options.parameter);
                }
            }

            if ("boolean" === typeof options.isJsonResponse) {
                isJsonResponse = options.isJsonResponse;
            }

            xhr.open(options.method, options.url + urlEncodedPar);

            if ("undefined" !== typeof options.headers) {
                Object.keys(options.headers).forEach(function(key) {
                    xhr.setRequestHeader(key, options.headers[key]);
                });
            }

            if ("function" === typeof options.onProgress) {
                xhr.upload.onprogress = options.onProgress;
            }

            xhr.onload = function() {
                var jsonRsp = null;

                if (200 !== xhr.status) {
                    if (true === isJsonResponse) {
                        jsonRsp = JSON.parse(xhr.response);
                        reject(jsonRsp);
                    } else {
                        reject(xhr.response);
                    }
                } else {
                    if (true === isJsonResponse) {
                        jsonRsp = JSON.parse(xhr.response);

                        if ("ok" === jsonRsp.status) {
                            resolve(jsonRsp);
                        } else {
                            reject(jsonRsp);
                        }
                    } else {
                        resolve(xhr.response);
                    }
                }
            };

            xhr.ontimeout = function() {
                console.error(xhr.statusText);
                reject("Timeout");
            };

            xhr.onerror = function() {
                console.error(xhr.statusText);
                reject("Error");
            };

            if (null === formData) {
                xhr.send();
            } else {
                xhr.send(formData);
            }
        }
    });
};
