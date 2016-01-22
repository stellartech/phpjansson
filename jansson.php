<?php

/*
 * Used for your IDE and documents the internal Jansson\Jansson class.
 * Do not include this at runtime, just have in your project for your
 * IDE to read and provide tooltips etc.
 */

namespace Jansson;

class Jansson
{
    /**
     * If $init is an array it will be used to set key/value pairs. 
     * If $init is a PHP stream byte stream of JSON it will be read 
     * and stored. Should always read a JSON object of key/value pairs. 
     * If the stream is invalid JSON then an exception will be thrown.
     * 
     * @param mixed $init Used to set key/value pairs
     * @throws JanssonConstructorException
     */
    public function __construct($init = null) {}
    
    /**
     * Test to see if a key/value pair exists.
     * 
     * @param string $key
     * @return bool true if $key exists or false otherwise.
     */
    public function has($key) {}
    
    /**
     * Implements the Countable interface. Returns the number
     * of top level key/value pairs (not including any sub-objects).
     * 
     * @return integer
     */
    public function count() {}
    
    /**
     * Set a key/value pair
     * 
     * @param string $key
     * @param mixed $value
     */
    public function set($key, $value) {}
    
    /**
     * Get a value for a key. If $exc is true (default) this method
     * will throw an exception if the key is not found. Setting $exc 
     * to false will inhibit this. Then on key not found returns null.
     * 
     * @param string $key
     * @param bool $exc
     * @return mixed value
     * @throws JanssonGetException if key does not exist
     */
    public function get($key, $exc = true) {}
    
    /**
     * Delete a key/value pair.
     * 
     * @param string $string
     */
    public function del($string) {}
    
    /**
     * Return the key/value pairs as a PHP array.
     */
    public function to_array() {}
    
    /**
     * Writes the key/value pairs as JSON to the PHP stream.
     * 
     * @param resource $stream The stream to output the JSON to.
     */
    public function to_stream($stream) {}
    
    /**
     * Reads the JSON byte stream from $stream and stores the key/value
     * pairs. Should always read a JSON object.
     * 
     * @param type $stream
     * @return bool true on sucess false on failure.
     */
    public function from_stream($stream) {}
    
}
