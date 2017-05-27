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
$j->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 4);

$k = new Jansson;
$k->from_string($j->to_string());
$k->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 4);

$arr['new'] = [
    'albert' => 'einstien',
    'isaac' => 'newton',
];

$m = new Jansson($arr);
$m->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 8);

?>
--EXPECT--
{
    "foo": "foo_value",
    "bar": "bar_value",
    "baz": "baz_value",
    "arr": {
        "baz": "baz_value",
        "bar": "bar_value",
        "foo": "foo_value"
    }
}{
    "foo": "foo_value",
    "bar": "bar_value",
    "baz": "baz_value",
    "arr": {
        "baz": "baz_value",
        "bar": "bar_value",
        "foo": "foo_value"
    }
}{
        "foo": "foo_value",
        "bar": "bar_value",
        "baz": "baz_value",
        "arr": {
                "baz": "baz_value",
                "bar": "bar_value",
                "foo": "foo_value"
        },
        "new": {
                "albert": "einstien",
                "isaac": "newton"
        }
}
