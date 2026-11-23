// FRACTALSCREEN license server, run by Cloudflare Pages ("advanced mode" _worker.js).
// Works with drag-and-drop uploads from the Cloudflare dashboard.
//   GET /api/verify?key=sub_...   -> { active, status, until_unix, plan }   (Windows Premium + Premium Web Player)
//   GET /api/license?txn=txn_...  -> { key, active, until_unix, plan }      (download page, right after checkout)
// Every other request is served from the website files as normal.
// Environment variables (Pages project > Settings > Variables and secrets):
//   PADDLE_API_KEY   your Paddle API key (store it as a secret)
//   PADDLE_ENV       "production" or "sandbox"
//   PRICE_BUSINESS   optional: your Business price ID, so Business customers are labelled correctly

const ACTIVE = ["active", "trialing", "past_due"]; // past_due: Paddle is still retrying the payment

async function paddle(env, path) {
  const base = env.PADDLE_ENV === "sandbox" ? "https://sandbox-api.paddle.com" : "https://api.paddle.com";
  const r = await fetch(base + path, { headers: { Authorization: "Bearer " + env.PADDLE_API_KEY } });
  if (r.status === 404) return null;
  if (!r.ok) throw new Error("Paddle error " + r.status);
  return (await r.json()).data;
}

function summary(sub, env) {
  const ends = sub.current_billing_period && sub.current_billing_period.ends_at;
  const until = ends ? Math.floor(Date.parse(ends) / 1000) : 0;
  const priceIds = (sub.items || []).map((i) => i.price && i.price.id);
  return {
    active: ACTIVE.includes(sub.status) && until > 0,
    status: sub.status,
    until_unix: until,
    plan: env.PRICE_BUSINESS && priceIds.includes(env.PRICE_BUSINESS) ? "business" : "premium",
  };
}

function json(obj, status = 200, cors = false) {
  const headers = { "content-type": "application/json", "cache-control": "no-store" };
  if (cors) headers["access-control-allow-origin"] = "*";
  return new Response(JSON.stringify(obj), { status, headers });
}

async function verify(url, env) {
  const key = (url.searchParams.get("key") || "").trim();
  if (!/^sub_[a-z0-9]+$/.test(key)) return json({ active: false, error: "invalid_key" }, 404, true);
  const sub = await paddle(env, "/subscriptions/" + key);
  if (!sub) return json({ active: false, error: "not_found" }, 404, true);
  return json(summary(sub, env), 200, true);
}

async function license(url, env) {
  const txn = (url.searchParams.get("txn") || "").trim();
  if (!/^txn_[a-z0-9]+$/.test(txn)) return json({ error: "invalid_transaction" }, 400);
  const t = await paddle(env, "/transactions/" + txn);
  if (!t) return json({ error: "not_found" }, 404);
  if (!t.subscription_id) return json({ pending: true }, 202);
  const sub = await paddle(env, "/subscriptions/" + t.subscription_id);
  if (!sub) return json({ pending: true }, 202);
  return json(Object.assign({ key: sub.id }, summary(sub, env)));
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/api/verify" || url.pathname === "/api/license") {
      if (request.method !== "GET") return json({ error: "method_not_allowed" }, 405);
      if (!env.PADDLE_API_KEY) return json({ error: "not_configured" }, 503, true);
      try {
        return url.pathname === "/api/verify" ? await verify(url, env) : await license(url, env);
      } catch (e) {
        return json({ error: "server" }, 502, true);
      }
    }
    return env.ASSETS.fetch(request);
  },
};
