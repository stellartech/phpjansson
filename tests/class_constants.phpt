--TEST--
Test get as PHP array
--SKIPIF--
<?php if (!extension_loaded("jansson")) print "skip"; ?>
--FILE--
<?php
$expect = [
    "JSON_COMPACT",
    "JSON_ENSURE_ASCII",
    "JSON_SORT_KEYS", 
    "JSON_PRESERVE_ORDER", 
    "JSON_ENCODE_ANY", 
    "JSON_ESCAPE_SLASH", 
    "JSON_REJECT_DUPLICATES", 
    "JSON_DECODE_ANY", 
    "JSON_DISABLE_EOF_CHECK", 
    "JSON_DECODE_INT_AS_REAL", 
    "JSON_ALLOW_NUL",
];

$actual = [];
$reflect = new ReflectionClass("Jansson\Jansson");

foreach($expect as $str) {
    $actual[$str] = $reflect->hasConstant($str) ? ($str."->true") : ($str."->false");
}

print_r($actual);
?>
--EXPECT--
Array
(
    [JSON_COMPACT] => JSON_COMPACT->true
    [JSON_ENSURE_ASCII] => JSON_ENSURE_ASCII->true
    [JSON_SORT_KEYS] => JSON_SORT_KEYS->true
    [JSON_PRESERVE_ORDER] => JSON_PRESERVE_ORDER->true
    [JSON_ENCODE_ANY] => JSON_ENCODE_ANY->true
    [JSON_ESCAPE_SLASH] => JSON_ESCAPE_SLASH->true
    [JSON_REJECT_DUPLICATES] => JSON_REJECT_DUPLICATES->true
    [JSON_DECODE_ANY] => JSON_DECODE_ANY->true
    [JSON_DISABLE_EOF_CHECK] => JSON_DISABLE_EOF_CHECK->true
    [JSON_DECODE_INT_AS_REAL] => JSON_DECODE_INT_AS_REAL->true
    [JSON_ALLOW_NUL] => JSON_ALLOW_NUL->true
)