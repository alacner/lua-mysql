require "mysql"

local db, h = mysql.connect('localhost', 'root', 'kernelx', 'testdb', 3306)
print(db)
print(h)
print('------------------------')
local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb')
--local db, err = mysql.connect('localhost:3308', 'root', 'kernel')
--local db, h = mysql.connect('localhost', 'root', 'kernel', 'testdb', 3306)
print(db)
print(err)

print('------------------------')
local db, err = mysql.connect('localhost', 'root', 'kernel')
print(db)
db:close()
print(db)
print(err)
--db:close()
--mysql.query(db, 'delete from `testdb`.`table` where `testdb`.`table`.`id` > 2')
