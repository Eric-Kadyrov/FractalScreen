<?php
// Shared helpers for the FRACTALSCREEN license server (verify.php, license.php).
// Not reachable from the web (see .htaccess).

const FS_ACTIVE_STATUSES = ['active', 'trialing', 'past_due']; // past_due: Stripe is still retrying the payment

function fs_config(): array {
    static $cfg = null;
    if ($cfg === null) {
        $file = __DIR__ . '/config.php';
        $cfg = is_file($file) ? (require $file) : [];
        if (!is_array($cfg)) $cfg = [];
        $env = getenv('STRIPE_SECRET_KEY');
        if ($env) $cfg['stripe_secret_key'] = $env;
    }
    return $cfg;
}

function fs_json($data, int $code = 200, bool $cors = true): void {
    http_response_code($code);
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    if ($cors) header('Access-Control-Allow-Origin: *');
    echo json_encode($data, JSON_UNESCAPED_SLASHES);
    exit;
}

function fs_require_configured(): string {
    $key = (string)(fs_config()['stripe_secret_key'] ?? '');
    if ($key === '' || strpos($key, 'REPLACE_ME') !== false) fs_json(['active' => false, 'error' => 'not_configured'], 503);
    return $key;
}

// Simple per-visitor limit so the endpoints can't be hammered.
function fs_rate_limit(string $bucket, int $max, int $seconds = 3600): void {
    $ip = $_SERVER['REMOTE_ADDR'] ?? 'unknown';
    $file = sys_get_temp_dir() . '/fs_rl_' . $bucket . '_' . hash('sha256', $ip);
    $now = time();
    $hits = [];
    if (is_file($file)) {
        $hits = array_filter(array_map('intval', explode(',', (string)@file_get_contents($file))), fn($t) => $t > $now - $seconds);
    }
    if (count($hits) >= $max) fs_json(['error' => 'rate_limited'], 429);
    $hits[] = $now;
    @file_put_contents($file, implode(',', $hits), LOCK_EX);
}

// GET a Stripe API object. Returns [httpStatus, decodedBody|null].
function stripe_get(string $path, array $query = []): array {
    $key = fs_require_configured();
    $base = rtrim((string)(fs_config()['stripe_api_base'] ?? 'https://api.stripe.com/v1'), '/');
    $url = $base . '/' . ltrim($path, '/') . ($query ? '?' . http_build_query($query) : '');
    $ch = curl_init($url);
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_USERPWD => $key . ':',
        CURLOPT_TIMEOUT => 15,
        CURLOPT_CONNECTTIMEOUT => 8,
        CURLOPT_HTTPHEADER => ['Accept: application/json'],
    ]);
    $body = curl_exec($ch);
    $status = (int)curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    if ($body === false || $status === 0) return [0, null];
    return [$status, json_decode($body, true)];
}

// End of the period the customer has paid for (works with old and new Stripe API versions).
function fs_period_end(array $sub): int {
    if (!empty($sub['current_period_end'])) return (int)$sub['current_period_end'];
    $end = 0;
    foreach (($sub['items']['data'] ?? []) as $item) {
        $end = max($end, (int)($item['current_period_end'] ?? 0));
    }
    if (!$end && !empty($sub['trial_end'])) $end = (int)$sub['trial_end'];
    return $end;
}

function fs_summary(array $sub): array {
    $until = fs_period_end($sub);
    $status = (string)($sub['status'] ?? '');
    $business = (array)(fs_config()['business_price_ids'] ?? []);
    $plan = 'premium';
    foreach (($sub['items']['data'] ?? []) as $item) {
        if (in_array($item['price']['id'] ?? '', $business, true)) $plan = 'business';
    }
    return [
        'active' => in_array($status, FS_ACTIVE_STATUSES, true) && $until > 0,
        'status' => $status,
        'until_unix' => $until,
        'plan' => $plan,
    ];
}
