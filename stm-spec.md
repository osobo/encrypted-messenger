
# Spec for  STM-protocol
__(Simple Tagged Messaging)__


The protocol is really simple and entirely text based. A message begins with any number of tags and ends with a variable length message body. Everyting (including the message body itself) is ASCII encoded, so no unicode (simple).

A tag is represented by a key value pair. The key may not contain any whitespaces. The value may contain spaces but no linebreaks. The structure is as follows: `key the value`. The key value pair is ended by a line break (`\n` / `0x0A`). For example, to specify the author using a tag called "author" would look like `author John Smith`. The tag keys should be interpreted in a non case sensitive way.

After all the line break terminated tags there should be a final extra line break after which comes the message body. Since the body of the message can contain line breaks the body is not terminated by a line break but by a null byte. This means that the whole message, with tags and body, is one long null terminated ascii string. If a message is written without any tags it would start with a single line break followed by the message body.

An example message could look like:
```
author John Smith
subject Important message

Lorem ipsum dolor sit amet, viderer democritum id has, fuisset verterem nec in, facer bonorum perfecto ei sea. Eu choro accusamus deseruisse has, mei ut alienum maluisset. Mea cu ornatus delicatissimi, sit elaboraret necessitatibus cu, ut partem albucius periculis pri. Libris alterum te mel, veri legere rationibus at pro. Scaevola detraxit vix ad, quo eu adhuc laudem.

Tantas consequat interesset ius in, no fugit reprehendunt mei. Eu detracto lucilius mea, ut eum augue sapientem. Ei latine quaeque mentitum vel. Duo aeterno salutatus theophrastus no. Ad hinc alterum sea, per et cetero malorum accusamus.

Ad eam summo electram. Te oratio laudem scriptorem his, cum ea summo periculis, ea mel consequat persecuti. Ne vim oblique ornatus honestatis, iuvaret laboramus intellegam ne mea, cum modus iusto eligendi te. Pro in malis homero tamquam, etiam intellegebat eam in. Ius te omnium posidonium. Errem bonorum assueverit vel ut, ad mollis forensibus cum.

Nulla democritum eloquentiam ei est, no summo maiorum constituam nec. Id solum dicant periculis vis, aliquip posidonium est eu, et natum lorem scripta vel. Eu pro blandit adolescens sententiae. An decore nostro accusam qui, an cum minim utinam voluptua. Mutat novum everti et cum, ad essent diceret nusquam qui. Duo simul omnesque ut, eam congue adipiscing honestatis ex.

Ea sale disputationi qui, has at postea inimicus. Ei vim wisi tollit quodsi, melius persecuti efficiantur in vim. Case tamquam percipitur ut vel, id per alterum delicata. Paulo mucius ei est. Et nibh solet sit, vel porro simul theophrastus ex. Elitr graece te eum, feugiat torquatos vulputate ius te, volutpat dissentiunt eu per.
```

Here's an example of a message with no tags.
```

Short message goes here.
```

When transmitting both of the above examples the ascii data would be followed by a single null byte to indicate the end of the message.

All ascii data (in tag keys, tag values and in the body) may only consist of _printable ascii characters_, ie only bytes in the range 0x20 - 0x7E. The only exception to this is the line break (0x0A) used to seperate tags or used anywhere in the body and the terminating null byte (0x0A).

So:

+ Tag keys and values are seperated by space (0x20).
+ Tag keys may consist of any _printable ascii character_ except space (0x20).
+ Tag values may consts of any _printable ascii character_.
+ The message body begins after the first line break (0x0A) that is not a tag terminator.
+ The message body may consist of any _printable ascii character_ __plus__ line breaks (0x0A).
+ The message body is terminated by a null byte (0x00).

(_printable ascii characters = any character in the range 0x20 - 0x7E).

## TODO
### 0 - Name
Should it be called _STM protocol_?


