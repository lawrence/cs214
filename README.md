# cs214
Assignments from CS214: Systems Programming during Fall 2015.

## pa5: Multi-Threading Bank System
By Chris McDonugh and Lawrence Park. (190/210 points)

This project was done multi-threaded between server and client pseudo bank transactions.

Some important project specifications were:
- 20 accounts in the bank only.
- 2 second timer to check if you can mutex lock the account (basic analogy is  claim the bathroom key so you can use the bathroom and no one else can)
- Client functions: open an account, start the session for a particular bank account, debit/credit money into the account or check the balance, finish the session of the bank account, and exit the bank server.  
- 20 second server check of status of all bank accounts printed on the server side.

We ended up doing very well on this project. The -20 point deduction was from a design standpoint where the syllabus for the project clearly didn't specify that debit transactions could also also add into the bank accounts. Instructor Russel told me that it was common sense and therefore did not give us points back...
