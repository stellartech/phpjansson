--TEST--
Test Jansson object.
--SKIPIF--
<?php if (!extension_loaded("jansson")) print "skip"; ?>
--FILE--
<?php
use Jansson\Jansson;

// Deliberately incorrectly formatted JSON produces PHP warning.
$k = new Jansson('{"foo:"Bar"}');
unset($k);
?>
--EXPECT--
Warning: Jansson\Jansson::__construct(): Jansson::__construct() failed: '':' expected near 'Bar'' at line 1, column 10, pos 10 in /workspace/tests/errors.php on line 5 
