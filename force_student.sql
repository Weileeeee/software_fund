UPDATE Users SET role = 'Student', block = 'Block A' WHERE username LIKE '%student%';
SELECT * FROM Users WHERE role = 'Student';
