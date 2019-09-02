## Greylist Policy Filter

**NOTE: This code is published for archival purposes and is not maintained. Do not used in production.**

A small C application for performing greylist policy checks with the Postfix MTA. Wikipedia describes it best as "Greylisting is a method of defending e-mail users against spam. A mail transfer agent (MTA) using greylisting will "temporarily reject" any email from a sender it does not recognize. If the mail is legitimate the originating server will try again after a delay, and if sufficient time has elapsed the email will be accepted."

This application manages the database and response to affect that behavior. There is also a Perl based utility which is used for performing maintenance operations on the database.