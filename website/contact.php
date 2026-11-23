<?php
// FRACTALSCREEN contact form: emails visitors' messages to contact@fractalscreen.com.
// Runs on Hostinger (PHP). The mailbox contact@fractalscreen.com must exist in Hostinger > Emails.

const TO_ADDRESS   = 'contact@fractalscreen.com';
const FROM_ADDRESS = 'contact@fractalscreen.com';   // must be an address on your own domain
const MAX_PER_HOUR = 5;                              // messages allowed per visitor per hour

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');

function reply($ok, $error = '', $code = 200) {
    http_response_code($code);
    echo json_encode($ok ? ['ok' => true] : ['ok' => false, 'error' => $error]);
    exit;
}

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') reply(false, 'method', 405);

// Only accept posts from this website
$origin = $_SERVER['HTTP_ORIGIN'] ?? ($_SERVER['HTTP_REFERER'] ?? '');
$host   = strtolower(preg_replace('/:\d+$/', '', $_SERVER['HTTP_HOST'] ?? ''));
$from   = strtolower((string)parse_url($origin, PHP_URL_HOST));
$strip  = fn($h) => preg_replace('/^www\./', '', $h);   // fractalscreen.com and www.fractalscreen.com both count
if ($origin !== '' && $strip($from) !== $strip($host)) reply(false, 'origin', 403);

// Hidden field that people never fill in, but spam robots do
if (trim($_POST['website'] ?? '') !== '') reply(true);

$name    = trim(str_replace(["\r", "\n"], ' ', $_POST['name'] ?? ''));
$email   = trim(str_replace(["\r", "\n"], '', $_POST['email'] ?? ''));
$topic   = trim(str_replace(["\r", "\n"], ' ', $_POST['topic'] ?? ''));
$message = trim($_POST['message'] ?? '');

if ($name === '' || mb_strlen($name) > 100)            reply(false, 'name', 422);
if (!filter_var($email, FILTER_VALIDATE_EMAIL) || strlen($email) > 200) reply(false, 'email', 422);
if (mb_strlen($message) < 5 || mb_strlen($message) > 5000) reply(false, 'message', 422);
if (mb_strlen($topic) > 60) $topic = mb_substr($topic, 0, 60);

// Simple per-visitor limit, so the form can't be used to flood the inbox
$ip   = $_SERVER['REMOTE_ADDR'] ?? 'unknown';
$file = sys_get_temp_dir() . '/fs_contact_' . hash('sha256', $ip);
$now  = time();
$hits = [];
if (is_file($file)) {
    $hits = array_filter(array_map('intval', explode(',', (string)@file_get_contents($file))), fn($t) => $t > $now - 3600);
}
if (count($hits) >= MAX_PER_HOUR) reply(false, 'limit', 429);
$hits[] = $now;
@file_put_contents($file, implode(',', $hits), LOCK_EX);

$subject = 'FRACTALSCREEN contact' . ($topic !== '' ? ": $topic" : '') . " from $name";
$body = "Name:    $name\n"
      . "Email:   $email\n"
      . ($topic !== '' ? "Topic:   $topic\n" : '')
      . "Sent:    " . gmdate('Y-m-d H:i') . " UTC\n"
      . "\n$message\n";

$headers = [
    'From: FRACTALSCREEN website <' . FROM_ADDRESS . '>',
    'Reply-To: ' . $name . ' <' . $email . '>',
    'MIME-Version: 1.0',
    'Content-Type: text/plain; charset=UTF-8',
    'Content-Transfer-Encoding: 8bit',
];
$encodedSubject = '=?UTF-8?B?' . base64_encode($subject) . '?=';

$sent = @mail(TO_ADDRESS, $encodedSubject, $body, implode("\r\n", $headers), '-f' . FROM_ADDRESS);
if (!$sent) reply(false, 'send', 500);
reply(true);
