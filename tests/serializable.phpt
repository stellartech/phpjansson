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

$s = $j->serialize($j);
unset($j);

echo "$s\n";

$n = new Jansson;
$n->unserialize($s);
print_r($n);
?>
--EXPECT--
{"foo":"foo_value","bar":"bar_value","baz":"baz_value","arr":{"baz":"baz_value","bar":"bar_value","foo":"foo_value"}}
Jansson\Jansson Object
(
    [[Internal Janson details]] => Array
        (
            [foo] => foo_value
            [bar] => bar_value
            [baz] => baz_value
            [arr] => Array
                (
                    [baz] => baz_value
                    [bar] => bar_value
                    [foo] => foo_value
                )

        )

)
