/* FRACTALSCREEN contact form.
   Any link with a data-contact attribute opens it (its value pre-fills the topic).
   Messages are sent by contact.php to contact@fractalscreen.com. */
(function () {
  var TO = "contact@fractalscreen.com";

  var css = document.createElement("style");
  css.textContent =
    "#fs-contact-dlg{width:min(520px,calc(100vw - 32px));max-height:calc(100dvh - 32px);overflow:auto;box-sizing:border-box;padding:28px;border:1px solid rgba(241,238,255,.18);border-radius:18px;background:#181B45;color:#F1EEFF;font-family:'Instrument Sans',system-ui,-apple-system,'Segoe UI',sans-serif;font-size:16px;line-height:1.5}" +
    "#fs-contact-dlg::backdrop{background:rgba(8,9,30,.72)}" +
    "#fs-contact-dlg h2{margin:0 0 6px;font-family:'Syne','Instrument Sans',sans-serif;font-weight:700;font-size:28px;line-height:1.15}" +
    "#fs-contact-dlg .fs-sub{margin:0 0 22px;color:#B5B3D6;font-size:15px}" +
    "#fs-contact-dlg form{display:flex;flex-direction:column;gap:16px;margin:0}" +
    "#fs-contact-dlg label{display:flex;flex-direction:column;gap:6px;font-size:15px;font-weight:600}" +
    "#fs-contact-dlg input,#fs-contact-dlg textarea{font:inherit;font-weight:400;color:#F1EEFF;background:#101233;border:1px solid #4A4F8C;border-radius:10px;padding:11px 14px;min-height:44px;box-sizing:border-box;width:100%}" +
    "#fs-contact-dlg textarea{min-height:140px;resize:vertical}" +
    "#fs-contact-dlg input:focus,#fs-contact-dlg textarea:focus{outline:2px solid #FFC27A;outline-offset:1px;border-color:transparent}" +
    "#fs-contact-dlg .fs-hp{position:absolute;left:-9999px;width:1px;height:1px;overflow:hidden}" +
    "#fs-contact-dlg .fs-row{display:flex;gap:12px;justify-content:flex-end;flex-wrap:wrap;margin-top:6px}" +
    "#fs-contact-dlg button{font:inherit;font-weight:600;min-height:48px;padding:0 24px;border-radius:999px;cursor:pointer}" +
    "#fs-contact-dlg .fs-send{border:0;background:#FFC27A;color:#101233}" +
    "#fs-contact-dlg .fs-send:disabled{opacity:.6;cursor:wait}" +
    "#fs-contact-dlg .fs-cancel{background:transparent;color:#F1EEFF;border:1.5px solid rgba(241,238,255,.45)}" +
    "#fs-contact-dlg .fs-status{margin:0;font-size:15px;min-height:1.5em}" +
    "#fs-contact-dlg .fs-status.err{color:#FFB4B4}#fs-contact-dlg .fs-status.ok{color:#9DF2C4}" +
    "#fs-contact-dlg .fs-status a{color:inherit}" +
    "#fs-contact-dlg button:focus-visible{outline:3px solid #FFC27A;outline-offset:3px}";
  document.head.appendChild(css);

  var dlg = document.createElement("dialog");
  dlg.id = "fs-contact-dlg";
  dlg.setAttribute("aria-labelledby", "fs-contact-title");
  dlg.innerHTML =
    '<h2 id="fs-contact-title">Contact us</h2>' +
    '<p class="fs-sub">Send us a message and we’ll reply by email, usually within a day.</p>' +
    '<form novalidate>' +
      '<label>Your name<input name="name" autocomplete="name" maxlength="100" required></label>' +
      '<label>Your email<input name="email" type="email" autocomplete="email" maxlength="200" required></label>' +
      '<label>Message<textarea name="message" maxlength="5000" required></textarea></label>' +
      '<input type="hidden" name="topic">' +
      '<div class="fs-hp" aria-hidden="true"><label>Website<input name="website" tabindex="-1" autocomplete="off"></label></div>' +
      '<p class="fs-status" role="status" aria-live="polite"></p>' +
      '<div class="fs-row"><button type="button" class="fs-cancel">Cancel</button><button type="submit" class="fs-send">Send message</button></div>' +
    '</form>';
  document.body.appendChild(dlg);

  var form = dlg.querySelector("form"), status = dlg.querySelector(".fs-status"),
      send = dlg.querySelector(".fs-send"), cancel = dlg.querySelector(".fs-cancel");

  function say(text, kind) { status.className = "fs-status" + (kind ? " " + kind : ""); status.innerHTML = text; }
  function close() { if (dlg.open) dlg.close(); }

  function open(topic) {
    form.reset(); say(""); send.disabled = false; send.textContent = "Send message"; send.hidden = false;
    cancel.textContent = "Cancel";
    form.elements.topic.value = topic || "";
    dlg.querySelector("h2").textContent = topic ? "Contact us: " + topic : "Contact us";
    if (typeof dlg.showModal === "function") dlg.showModal(); else dlg.setAttribute("open", "");
    form.elements.name.focus();
  }

  cancel.addEventListener("click", close);
  dlg.addEventListener("click", function (e) { if (e.target === dlg) close(); });   // click outside the card

  form.addEventListener("submit", function (e) {
    e.preventDefault();
    var f = form.elements;
    if (!f.name.value.trim()) { say("Please enter your name.", "err"); f.name.focus(); return; }
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(f.email.value.trim())) { say("Please enter a valid email address, so we can reply.", "err"); f.email.focus(); return; }
    if (f.message.value.trim().length < 5) { say("Please write a short message.", "err"); f.message.focus(); return; }
    send.disabled = true; send.textContent = "Sending…"; say("");
    fetch("contact.php", { method: "POST", body: new FormData(form) })
      .then(function (r) { return r.json().catch(function () { return { ok: false, error: "server" }; }); })
      .then(function (j) {
        if (j && j.ok) {
          say("Thank you! Your message has been sent. We’ll reply to " + f.email.value.trim().replace(/</g, "&lt;") + ".", "ok");
          send.hidden = true; cancel.textContent = "Close";
          return;
        }
        var err = new Error("send");
        if (j && j.error === "limit") err.userMsg = "You’ve sent several messages already. Please try again in an hour.";
        throw err;
      })
      .catch(function (err) {
        send.disabled = false; send.textContent = "Send message";
        var m = err && err.userMsg ? err.userMsg :
          "Sorry, the message couldn’t be sent. Please try again, or email us at <a href=\"mailto:" + TO + "\">" + TO + "</a>.";
        say(m, "err");
      });
  });

  // Links with data-contact open the form instead of the visitor's email app
  document.addEventListener("click", function (e) {
    var a = e.target.closest && e.target.closest("[data-contact]");
    if (!a) return;
    e.preventDefault();
    open(a.getAttribute("data-contact"));
  });
})();
