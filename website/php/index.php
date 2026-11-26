<?php

declare(strict_types=1);

// Shared-host setups can keep index.php first in DirectoryIndex.
readfile(__DIR__ . '/index.html');
