
# Spec for  I2C-ACL
__(I2C Asynchronous Communication Layer/)__


This text explains how a Master and Slave can use I2C to "asynchronously" communicate. This will give both of them the illusion of both being able to send to each other in both directions at any time.

The master will have to decide on a polling frequency. This could be 1 Hz, 2 Hz, 10 Hz 100 Hz, whatever.


## From the master POV

At the polling frequency the master will be calling `master_try_read()` which in turn might call `master_handle_read()`. Whenever the master wants to send something it should call `master_write()`.

### - `master_try_read()`

The function takes no arguments and returns nothing. First this function must check if there is currently a master write in progress. If there is this function should immediately exit. The function will send a start condition to the slave in master read mode. The Slave will respond with 2 bytes. The bytes should be interpreted as a 2 byte unsigned integer in big endian form. Let's call this integer `n`.

+ If `n = 0`, then the slave has nothing to send and the master will send a `NACK`, then a stop condition and finally can exit the function.

+ If `n > 0`, then the master will read `n` more bytes from the slave before `NACK`ing and sending stop. Before exiting the function the master must pass the `n` read bytes tp `master_handle_read()`.


### - `master_handle_read()`

This function takes one argument: an array of bytes that the slave has sent. This will be at most 65535 (2^16-1) and at least 1 byte in length. What the function does is up to the user of the protocol.


### - `master_write()`

This function takes an array of bytes as argument and returns nothing. The array should be between 1 and 65535 (2^16-1) in length. First this function must check if there is currently a master read in progress. If this is the case this function should block until that is no longer the case. Next the function should send a start condition to the slave in master write mode. Next the master will send all of the bytes in the passed array. Finally the master will send a stop condition.


### Notes

+ The master writes to slave by using the blocking function `master_write()`. When the function returns all of the passed bytes will have been sent to the slave.

+ The master reads from the slave using the handler `master_handle_read()`. The function will be called with the read data whenever the slave has sent something. If the master wants a blocking read function it could simply block until the handler is called.





## From the slave POV

The slave will let the function `slave_handle_i2c()` handle all start conditions comming from the master. To write to the master the slave will use the blocking function `slave_write()` and must define the `slave_handle_read()` function to handle incomming master writes.

### - `slave_write()`

This function will take one argument: an array of bytes. The length will be between 1 and 65535 (2^16-1) bytes in length. The function will write the bytes a globally accessable write buffer and set a flag called `pending_write` and block until the flag is disabled.


### - `slave_handle_read()`

This function takes one argument: an array of bytes that the master has sent. This will be at most 65535 (2^16-1) and at least 1 byte in length. What the function does is up to the user of the protocol.


### - `slave_handle_i2c()`

This function does not take any arguments but is called whenever the slave detects a start condition from the master.

+ If it is in master write mode the function will read from master until stop. The master will send at least 1 byte and at most 65535 (2^16-1) bytes. The read bytes will be sent to `slave_handle_read()`.

+ If it is in master read mode and the flag `pending_write` __is not__ set the slave will send two all 0 bytes. The master will `NACK` after the two bytes and then send a stop. Now the function can exit.

+ If it is in master read mode and the flag `pending_write` __is__ set: let `n` be the number of bytes in the write buffer. Send `n` to master as two bytes in big endian form. Next the slave will send the `n` bytes in the write buffer to the master. After the last byte has been sent the master will `NACK` and send stop. Now the function can disable `pending_write` and exit.


### Notes

+ The slave writes to master by using the blocking function `master_write()`. When the function returns all of the passed bytes will have been sent to the master.

+ The slave reads from the master using the handler `slave_handle_read()`. The function will be called with the read data whenever the master has sent something. If the slave wants a blocking read function it could simply block until the handler is called.





