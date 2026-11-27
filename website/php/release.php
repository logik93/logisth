<?php

declare(strict_types=1);

namespace Wavelogik;

const RELEASES_URL = 'https://github.com/logik93/logisth/releases';
const REPOSITORY_URL = 'https://github.com/logik93/logisth';
const MANUALS = [
    'user' => ['MANUAL.md', 'logisth-Manual-English.pdf'],
    'technical' => ['TECHNICAL_MANUAL.md', 'logisth-Technical-Manual-English.pdf'],
];
const LATEST_API = 'https://api.github.com/repos/logik93/logisth/releases/latest';
const MAX_RELEASE_BYTES = 1024 * 1024;

const PACKAGES = [
    'Windows' => ['windows-x86_64.zip', 'x64 · ZIP'],
    'Linux' => ['linux-x86_64.tar.gz', 'x86_64 · TAR.GZ'],
    'macOS' => ['macos-universal.zip', 'Universal · ZIP'],
];

function releaseDetails(mixed $data): ?array
{
    if (!is_array($data) || ($data['draft'] ?? true) !== false ||
        ($data['prerelease'] ?? true) !== false || !is_array($data['assets'] ?? null)) {
        return null;
    }

    $tag = $data['tag_name'] ?? null;
    if (!is_string($tag) || preg_match('/\Av\d+(?:\.\d+){2,3}\z/', $tag) !== 1) {
        return null;
    }

    $uploaded = [];
    foreach ($data['assets'] as $asset) {
        if (is_array($asset) && ($asset['state'] ?? null) === 'uploaded' &&
            is_string($asset['name'] ?? null)) {
            $uploaded[$asset['name']] = $asset['browser_download_url'] ?? null;
        }
    }

    $downloads = [];
    foreach (PACKAGES as $platform => [$suffix, $format]) {
        $name = 'logisth-' . substr($tag, 1) . '-' . $suffix;
        $url = RELEASES_URL . '/download/' . $tag . '/' . $name;
        $downloads[$platform] = [
            'format' => $format,
            'url' => ($uploaded[$name] ?? null) === $url ? $url : null,
        ];
    }

    $manuals = [];
    foreach (MANUALS as $key => [$markdown, $pdf]) {
        $url = RELEASES_URL . '/download/' . $tag . '/' . $pdf;
        $manuals[$key] = [
            'online' => REPOSITORY_URL . '/blob/' . $tag . '/docs/' . $markdown,
            'pdf' => ($uploaded[$pdf] ?? null) === $url ? $url : null,
        ];
    }

    return [
        'version' => substr($tag, 1),
        'url' => RELEASES_URL . '/tag/' . $tag,
        'downloads' => $downloads,
        'manuals' => $manuals,
    ];
}

function fetchRelease(): ?array
{
    $body = false;
    if (function_exists('curl_init')) {
        $curl = curl_init(LATEST_API);
        $buffer = '';
        curl_setopt_array($curl, [
            CURLOPT_CONNECTTIMEOUT => 2,
            CURLOPT_TIMEOUT => 4,
            CURLOPT_USERAGENT => 'wavelogik.pl',
            CURLOPT_HTTPHEADER => ['Accept: application/vnd.github+json'],
            CURLOPT_WRITEFUNCTION => static function ($handle, string $chunk) use (&$buffer): int {
                if (strlen($buffer) + strlen($chunk) > MAX_RELEASE_BYTES) {
                    return 0;
                }
                $buffer .= $chunk;
                return strlen($chunk);
            },
        ]);
        if (curl_exec($curl) !== false && curl_getinfo($curl, CURLINFO_HTTP_CODE) === 200) {
            $body = $buffer;
        }
    } elseif (filter_var(ini_get('allow_url_fopen'), FILTER_VALIDATE_BOOLEAN)) {
        $context = stream_context_create([
            'http' => [
                'header' => "User-Agent: wavelogik.pl\r\nAccept: application/vnd.github+json\r\n",
                'timeout' => 4,
                'follow_location' => 0,
            ],
        ]);
        $body = @file_get_contents(LATEST_API, false, $context, 0, MAX_RELEASE_BYTES + 1);
    }

    $data = is_string($body) && strlen($body) <= MAX_RELEASE_BYTES
        ? json_decode($body, true)
        : null;

    return is_array($data) ? $data : null;
}

function readReleaseCache(string $path): array
{
    $body = @file_get_contents($path, false, null, 0, MAX_RELEASE_BYTES + 512);
    $cache = is_string($body) ? json_decode($body, true) : null;

    return is_array($cache) ? $cache : [];
}

function cachedRelease(array $cached, bool $stale = false): ?array
{
    $release = releaseDetails($cached['data'] ?? null);
    if ($release !== null && ($stale || ($cached['stale'] ?? false))) {
        $release['stale'] = true;
    }

    return $release;
}

function latestRelease(string $directory, callable $fetch): ?array
{
    if (!is_dir($directory) && !@mkdir($directory, 0700, true) && !is_dir($directory)) {
        return releaseDetails($fetch());
    }

    $path = $directory . '/latest.json';
    $cached = readReleaseCache($path);
    if (is_int($cached['next_check'] ?? null) && $cached['next_check'] > time()) {
        return cachedRelease($cached);
    }

    $lock = @fopen($directory . '/refresh.lock', 'c');
    if ($lock === false) {
        return releaseDetails($fetch()) ?? cachedRelease($cached, true);
    }

    try {
        if (!flock($lock, LOCK_EX | LOCK_NB)) {
            return cachedRelease($cached, true);
        }

        $cached = readReleaseCache($path);
        if (is_int($cached['next_check'] ?? null) && $cached['next_check'] > time()) {
            return cachedRelease($cached);
        }

        $data = $fetch();
        $details = releaseDetails($data);
        $next = json_encode([
            'next_check' => time() + ($details === null ? 60 : 300),
            'stale' => $details === null,
            'data' => $details === null ? ($cached['data'] ?? null) : $data,
        ], JSON_UNESCAPED_SLASHES);

        // Readers see the previous complete response until the new one is ready.
        $temp = @tempnam($directory, 'release-');
        if ($temp !== false) {
            if ($next !== false && @file_put_contents($temp, $next) === strlen($next)) {
                @rename($temp, $path);
            }
            if (is_file($temp)) {
                @unlink($temp);
            }
        }

        return $details ?? cachedRelease($cached, true);
    } finally {
        fclose($lock);
    }
}
