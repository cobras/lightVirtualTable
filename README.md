# lightVirtualTable
Use sqllite virtual table instead of native table.

The goal of this project is to implement a virtual table to store data with structure that had lower size than sqllite rows.
When using virtual table, all index, and search had to be implemented.
Because my index for the virtual table I use is a date field, I decided to create a bitmap that will be filled each time I add a row.

