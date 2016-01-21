--TEST--
Test Jansson object.
--SKIPIF--
<?php if (!extension_loaded("jansson")) print "skip"; ?>
--FILE--
<?php
use Jansson\Jansson;
$fh = fopen("php://temp/maxmemory:1024", "rw");
if(!$fh) {
    echo "failed to create stream";
}
else {
    $a = array('foo1' => 'bar1');
    $j = new Jansson($a); // Tests ::__construct() with init from array
    $j->set("foo2", "bar2"); // Tests ::set()
    $j->set("foo3", "bar3"); // Tests ::set()
    $j->to_stream($fh); // Tests ::to_stream()
    unset($j); // Destroy original object
    
    // Read stream back in and test...
    rewind($fh);
    $k = new Jansson; // Tests ::__construct() with no init parameter
    $k->from_stream($fh); // Tests ::from_stream()    
    echo $k->get("foo1") == "bar1" ? "true1\n" : "false1\n"; // Tests ::get()
    echo $k->get("foo2") == "bar2" ? "true2\n" : "false2\n"; // Tests ::get()
    echo $k->get("foo3") == "bar3" ? "true3\n" : "false3\n"; // Tests ::get()
    echo count($k) === 3 ? "true4\n" : "false4\n"; // Tests Countable interface
    echo $k->del("foo3") === 0 ? "true5\n" : "false5\n"; // Tests ::del()
    echo $k->del("foo3") === -1 ? "true6\n" : "false6\n"; // Tests ::del()
    echo count($k) === 2 ? "true7\n" : "false7\n";
    echo $k->has("foo1") === true ? "true8\n" : "false8\n"; // Tests ::has()
    echo $k->has("bar1") === false ? "true9\n" : "false9\n"; // Tests ::has()
    try {
        // if key not found for ::get() an exception is thrown. We use an
        // exception because NULL, FALSE, 0, etc are all valid JSON types
        // which can be returned by ::get(). Note the second parameter to get() 
        // is not supplied and defaults to true (use exceptions)
        $k->get("bar1");
    }
    catch(JanssonGetException $e) {
        echo $e->getMessage() == "Jansson::get() Key not found" ? 
            "true10\n" : "false10\n";
    }
    
    // It is possible to use ::get() without exceptions. In this case supply 
    // false as the second parameter. However, null is returned if the key was
    // not found which makes it hard to know if JSON_NUL was the actual value 
    // or if the key simply did not exist at all.
    echo is_null($k->get("bar1", false)) ? "true11\n" : "false11\n";
    
    unset($k);
    
    // Read stream back in constructor and test...
    rewind($fh);
    $m = new Jansson($fh); // Test ::__construct with init from stream
    echo count($m) === 3 ? "true12\n" : "false12\n";
    $a = $m->to_array(); // Test ::to_array()
    echo isset($a["foo1"]) &&  $a["foo1"] == "bar1" ? "true20\n" : "false20\n";
    echo isset($a["foo2"]) &&  $a["foo2"] == "bar2" ? "true21\n" : "false21\n";
    echo isset($a["foo3"]) &&  $a["foo3"] == "bar3" ? "true22\n" : "false22\n";    
    unset($m);
    fclose($fh);
}

// Test that the constructor throws an exception if the init stream
// is invalid JSON input.
$fh = fopen("php://temp/maxmemory:1024", "rw");
if(!$fh) {
    echo "Failed to load json";
}
else {
    fwrite($fh, "This is not JSON!");
    rewind($fh);
    try {
        $m = new Jansson($fh);
    } 
    catch (JanssonConstructorException $e) {
        echo $e->getMessage() == "Jansson::__construct() failed to load from stream" ?
                "true30" : "false30";
    }
    fclose($fh);
}
?>
--EXPECT--
true1
true2
true3
true4
true5
true6
true7
true8
true9
true10
true11
true12
true20
true21
true22
true30
