"use strict";

/* Attach the namespace explicitly to window: in a classic (non-module) script a
 * top-level "const" does NOT create a global property, but every page relies on
 * dialog being globally available. */
const dialog = (window.dialog = window.dialog || {});

dialog._getModal = function () {
    return bootstrap.Modal.getOrCreateInstance(document.getElementById("modalDialog"));
};

dialog._createCloseButton = function () {
    const button = document.createElement("button");

    button.type = "button";
    button.className = "btn btn-secondary";
    button.setAttribute("data-bs-dismiss", "modal");
    button.textContent = "Ok";

    return button;
};

dialog._prepare = function (headerClass, footerButtons) {
    const header = document.getElementById("dialogHeader");
    const footer = document.getElementById("dialogFooter");

    header.className = headerClass;
    footer.replaceChildren(...footerButtons);
};

dialog._show = function (title, message, isBlocking) {
    return new Promise((resolve) => {
        const modalElement = document.getElementById("modalDialog");
        const waitOnClick = (isBlocking === true);

        document.getElementById("dialogTitle").textContent = title;
        document.getElementById("dialogBody").innerHTML = message;

        if (waitOnClick === false) {
            modalElement.addEventListener("shown.bs.modal", () => resolve(), { once: true });
        } else {
            /* Blocking: resolve on any secondary button. */
            modalElement.querySelectorAll(".btn-secondary").forEach((button) => {
                button.addEventListener("click", () => resolve(), { once: true });
            });
        }

        dialog._getModal().show();
    });
};

dialog.hide = function () {
    return new Promise((resolve) => {
        const modalElement = document.getElementById("modalDialog");

        modalElement.addEventListener("hidden.bs.modal", () => resolve(), { once: true });

        dialog._getModal().hide();
    });
};

dialog.showInfo = function (message, isBlocking) {
    dialog._prepare("modal-header bg-primary text-white", [dialog._createCloseButton()]);

    return dialog._show("Info", message, isBlocking);
};

dialog.showWarning = function (message, isBlocking) {
    dialog._prepare("modal-header bg-warning", [dialog._createCloseButton()]);

    return dialog._show("Warning", message, isBlocking);
};

dialog.showError = function (message, isBlocking) {
    dialog._prepare("modal-header bg-danger text-white", [dialog._createCloseButton()]);

    return dialog._show("Error", message, isBlocking);
};

dialog.show = function (title, message, isBlocking) {
    dialog._prepare("modal-header bg-dark text-white", []);

    return dialog._show(title, message, isBlocking);
};

dialog.updateMessage = function (message) {
    document.getElementById("dialogBody").innerHTML = message;
};
