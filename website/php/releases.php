<?php

declare(strict_types=1);

require __DIR__ . '/release.php';

use function Wavelogik\fetchRelease;
use function Wavelogik\latestRelease;

$cacheDirectory = getenv('WAVELOGIK_CACHE_DIR') ?: sys_get_temp_dir() . '/wavelogik-' . hash('sha256', __DIR__);
$release = latestRelease($cacheDirectory, fetchRelease(...));

header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');
header('Cache-Control: no-cache');
echo json_encode($release, JSON_THROW_ON_ERROR | JSON_UNESCAPED_SLASHES);
