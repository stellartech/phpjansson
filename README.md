# PHP Jansson
A wrapper class for the Jansson C library.

The PHP class Jansson\Jansson is a wrapper around a JSON object key/value
store. Unlike PHP's json\_encode() function Jansson\Jansson streams it's
encoded output direct to a PHP stream thus avoiding the memory consuming
process of converting a PHP array into a JSON string and then sending that
PHP string to output (which if output buffering is in use means sending a 
string memory buffer to yet another output memory buffer). Instead, the 
Jansson\Jansson class writes it's output chucked to the stream you specify.

Note, you need Jansson version 2.7 or greater to compile and use this wrapper.

https://github.com/akheron/jansson
http://jansson.readthedocs.org/en/latest

jansson.php is a "stub class" definition which provides the class method 
prototypes to assist your IDE in code completion, tooltips, etc.

See the tests/basic.phpt unit test file for instructions on how to use the PHP class.

    phpize
    ./configure --with-jansson
    make
    make install
    make test TESTS=.  [optional]

INI settings:-

    extension=jansson.so
    
    [jansson]
    seed=0
    use_php_memory=1

Jansson seeds it's random number generator for the key hashing. A seed of zero
ensures this. However, if you want to disable this and provide a known seed
for repeatability (such as unit testing) then you can set the seed value.

By default Jansson uses PHP's request cycle memory for it's allocations thus
limiting how much memory can be used for a Jansson object to that limited to 
your PHP request process. Setting this to 0 (zero) allows Jansson to use
standard malloc/free memory allocation and therefore removing PHP's memory 
limit and only restricting you to your server's memory limitations. Note,
this INI setting can only be set system wide and not a run time. An Apache/FPM
full restart is required when changing this setting.


## Usage

### Create a simple JANSSON object

```
<?php
use Jansson\Jansson;

$j = new Jansson;
$j->set("key1", "bar1");
$j->set("key2", "bar2");
$j->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 4);
```
The above snippet will output (insert order is preserved and indent of 4 spaces):-
```
{
    "key1": "bar1",
    "key2": "bar2"
}
```
_Note_, the Jansson::to\_stream() method is very memory efficient as the JSON stream is built directly to the output stream thus avoiding dymanic building a char buffer in memory to send to the stream on completion.

### Create a simple JANSSON object from an input stream

```
<?php
use Jansson\Jansson;

$j = new Jansson;
$j->from_stream(STDIN);
$j->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 4);
```
Run thus:-
```
$ echo '{"foo":"bar"}' | php example.php
```
will output thus:-
```
{
    "foo": "bar"
}
```
(_Note_, Jansson::from\_stream() is destructive for any elements that were previously added to the JANSSON object)

### Jansson and PHP arrays

```
<?php
use Jansson\Jansson;

$a = ['foo' => 'bar'];
$j = new Jansson($a);
$j->to_stream(STDOUT, Jansson::JSON_PRESERVE_ORDER, 4);
```
will output thus:-
```
{
    "foo": "bar"
}
```

Jansson also supports ::to\_array() to convert it's internal state into a PHP array.


### jansson.php

See the jansson.php file which is designed to assist your IDE with hints and tool tips.

### Serialize

Jansson supports Jansson::serialize() and Jansson::unserialize()

__Note__, this is not serialize/unserialize to PHP values. The serialization is done using JSON notation.

These allow writing the contents to/from PHP value strings only rather than streams or arrays.

### ToDo.

Support debug info such as print\_r() and var\_export.



