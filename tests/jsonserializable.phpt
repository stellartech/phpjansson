--TEST--
Test Jansson object.
--SKIPIF--
<?php if (!extension_loaded("jansson")) print "skip"; ?>
--FILE--
<?php
use Jansson\Jansson;
$arr = array(
    'foo' => 'foo_value',
    'bar' => 'bar_value',
    'baz' => 'baz_value',
    'arr' => array(
        'baz' => 'baz_value',
        'bar' => 'bar_value',
        'foo' => 'foo_value',
    ),
);
$j = new Jansson($arr);

$s = json_encode($j);
unset($j);

printf("$s");
?>
--EXPECT--
{"foo":"foo_value","bar":"bar_value","baz":"baz_value","arr":{"baz":"baz_value","bar":"bar_value","foo":"foo_value"}}
