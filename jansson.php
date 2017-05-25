<?php

/*
 * Used for your IDE and documents the internal Jansson\Jansson class.
 * Do not include this at runtime, just have in your project for your
 * IDE to read and provide tooltips etc.
 */

namespace Jansson;

class Jansson implements Serializable, Countable
{
    /**
     * If $init is an array it will be used to set key/value pairs. 
     * If $init is a PHP stream byte stream of JSON it will be read 
     * and stored. Should always read a JSON object of key/value pairs. 
     * If the stream is invalid JSON then an exception will be thrown.
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->set('foo', 'bar');
     * $j->to_stream(STDOUT);
     * ```
     * ```php 
     * use Jansson\Jansson;
     *
     * $j = new Jansson(STDIN);
     * $j->to_stream(STDOUT);
     * ``` 
     * ```php
     * use Jansson\Jansson;
     * $a = ['foo' => 'bar'];
     * $j = new Jansson($a);
     * $j->to_stream(STDOUT);
     * ```
     *
     * @param mixed $init Used to set key/value pairs
     * @throws JanssonConstructorException
     *
     * @\Jansson\Jansson::__construct()
     *
     * @return Jansson object
     */
    public function __construct($init = null) {}
    
    /**
     * Test to see if a key/value pair exists.
     *
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson(STDIN);
     * if($j->has('foo')) echo "foo was a valid key\n";
     * ```
     * 
     * @param string $key
     *
     * @\Jansson\Jansson::has()
     *
     * @return bool true if $key exists or false otherwise.
     */
    public function has($key) {}
    
    /**
     * Implements the Countable interface. Returns the number
     * of top level key/value pairs (not including any sub-objects).
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson(STDIN);
     * $i = $j->count();
     * echo "stdin brought $i keys to teh party\n";
     * ```
     *
     * @\Jansson\Jansson::count()
     *
     * @return integer
     */
    public function count() {}
    
    /**
     * Set a key/value pair
     *
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->set('foo', 'bar');
     * $j->to_stream(STDOUT);
     * ```
     * 
     * @param string $key
     *
     * @\Jansson\Jansson::set()
     *
     * @param mixed $value
     */
    public function set($key, $value) {}
    
    /**
     * Get a value for a key. If $exc is true (default) this method
     * will throw an exception if the key is not found. Setting $exc 
     * to false will inhibit this. Then on key not found returns null.
     * Note, JSON has a defined NULL type. Use exceptions if JSON NULL
     * types are expected otherwise telling the difference between a
     * valid of invalid value is not possible as this method will return
     * PHP null if not found and not using exceptions.
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson(STDIN);
     * try {
     *     $var = $j->get('foo');
     *     echo $var;
     * } 
     * catch(JanssonGetException $e) {
     *     echo "key 'foo' didn't exist\n";
     * }
     * ```
     * @param string $key
     * @param bool $exc
     *
     * @\Jansson\Jansson::get()
     *
     * @return mixed value
     * @throws JanssonGetException if key does not exist
     */
    public function get($key, $exc = true) {}
    
    /**
     * Delete a key/value pair.
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->set('foo', 'bar');
     * echo "Have {$j->count()} keys"; // "Have 1 keys"
     * $j->del('foo');
     * echo "Have {$j->count()} keys"; // "Have 0 keys"
     * ```
     *
     * @param string $key
     *
     * The return value can be:-
     *   php bool false, internal Jansson object error
     *   php int, 0 is success, -1 key not found.
     *
     * @\Jansson\Jansson::del()
     *
     * @return mixed
     */
    public function del($key) {}
    
    /**
     * Converts internal JSON representation to PHP array.
     *
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->set('foo', 'bar');
     * $a = $j->to_array();
     * var_export($a);
     * ```
     *
     * @\Jansson\Jansson::to_array();
     *
     * @return array
     */
    public function to_array() {}
    
    /**
     * Loads self from PHP array.
     *
     * ```php
     * use Jansson\Jansson;
     * $a = ['foo' => 'bar'];
     * $j = new Jansson;
     * $j->from_array($a);
     * print_r($j);
     * ```
     *
     * @\Jansson\Jansson::from_array();
     *
     * @return array
     */
    public function from_array() {}
    
    /**
     * Writes the key/value pairs as JSON to the PHP stream.
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->set('foo', 'bar');
     * $j->to_stream(STDOUT);
     * ```
     *
     * @\Jansson\Jansson::to_stream();
     *
     * @param resource $stream The stream to output the JSON to.
     */
    public function to_stream($stream) {}
    
    /**
     * Reads the JSON byte stream from $stream and stores the key/value
     * pairs. Should always read a JSON object.
     * Note, this operation is destructive for any existing key/value
     * pairs already inside the object.
     * 
     * ```php
     * use Jansson\Jansson;
     * $j = new Jansson;
     * $j->from_stream(STDIN);
     * ```
     * @param type $stream
     *
     * @\Jansson\Jansson::from_stream();
     *
     * @return bool true on sucess false on failure.
     */
    public function from_stream($stream) {}
    
}

