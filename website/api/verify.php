<?php
// GET /api/verify.php?key=sub_...  ->  {"active":true,"status":"active","until_unix":1767225600,"plan":"premium"}
// Used by the Windows Premium screensaver and the Premium Web Player.
// The license key is the customer's Stripe subscription ID.
require __DIR__ . '/stripe.php';

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') fs_json(['ok' => true]);
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') fs_json(['error' => 'method_not_allowed'], 405);

$key = trim((string)($_GET['key'] ?? ''));
if (!preg_match('/^sub_[A-Za-z0-9]{6,80}$/', $key)) fs_json(['active' => false, 'error' => 'invalid_key'], 404);

fs_rate_limit('verify', 120);

// Short cache, so many checks of the same key don't all go to Stripe
$cacheFile = sys_get_temp_dir() . '/fs_sub_' . hash('sha256', $key);
if (is_file($cacheFile) && filemtime($cacheFile) > time() - 300) {
    $cached = json_decode((string)@file_get_contents($cacheFile), true);
    if (is_array($cached)) fs_json($cached);
}

[$status, $sub] = stripe_get('subscriptions/' . rawurlencode($key));
if ($status === 404) fs_json(['active' => false, 'error' => 'not_found'], 404);
if ($status !== 200 || !is_array($sub)) fs_json(['error' => 'server'], 502);

$result = fs_summary($sub);
@file_put_contents($cacheFile, json_encode($result), LOCK_EX);
fs_json($result);
