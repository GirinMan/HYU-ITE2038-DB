SELECT name
FROM Pokemon
WHERE type = 'Grass'
ORDER BY name;

SELECT name
FROM Trainer
WHERE hometown = 'Brown City' OR hometown = 'Rainbow City'
ORDER BY name;

SELECT DISTINCT type
FROM Pokemon
ORDER BY type;

SELECT name
FROM City
WHERE name LIKE 'B%'
ORDER BY name;

SELECT hometown
FROM Trainer
WHERE name NOT LIKE 'M%'
ORDER BY hometown;

SELECT nickname
FROM CatchedPokemon
WHERE level = (SELECT MAX(level) FROM CatchedPokemon)
ORDER BY nickname;

SELECT name
FROM Pokemon
WHERE (
  name LIKE 'A%' OR
  name LIKE 'E%' OR
  name LIKE 'I%' OR
  name LIKE 'O%' OR
  name LIKE 'U%'
)
ORDER BY name;
  
SELECT AVG(level)
FROM CatchedPokemon;

SELECT MAX(level)
FROM CatchedPokemon
WHERE owner_id = (SELECT id FROM Trainer WHERE name = 'Yellow');

SELECT DISTINCT hometown
FROM Trainer
ORDER BY hometown;

SELECT Trainer.name, CatchedPokemon.nickname
FROM CatchedPokemon
JOIN Trainer ON CatchedPokemon.owner_id = Trainer.id
WHERE CatchedPokemon.nickname LIKE 'A%'
ORDER BY Trainer.name;

SELECT t.name
FROM Trainer t, Gym g, City c
WHERE t.id = g.leader_id AND c.name = g.city AND c.description = 'Amazon';

SELECT *
FROM(
  SELECT Trainer.id, COUNT(*) cnt
  FROM CatchedPokemon
  JOIN Trainer ON CatchedPokemon.owner_id = Trainer.id
  JOIN Pokemon ON CatchedPokemon.pid = Pokemon.id
  WHERE Pokemon.type = 'Fire'
  GROUP BY Trainer.name
) AS T1
ORDER BY T1.cnt DESC LIMIT 1;

SELECT DISTINCT type
FROM Pokemon
WHERE id < 10
ORDER BY id DESC;

SELECT COUNT(*)
FROM Pokemon
WHERE type != 'Fire';

SELECT p.name
FROM Evolution e
JOIN Pokemon p ON e.before_id = p.id
WHERE e.before_id > e.after_id
ORDER BY p.name;

SELECT AVG(c.level)
FROM CatchedPokemon c
JOIN Pokemon p ON c.pid = p.id
WHERE p.type = 'Water';

SELECT cp.nickname
FROM CatchedPokemon cp
WHERE cp.level = (
  SELECT MAX(level)
  FROM CatchedPokemon c
  JOIN Gym g ON c.owner_id = g.leader_id
);

SELECT T1.name
FROM (
SELECT t.name, AVG(c.level) var1
FROM CatchedPokemon c, Trainer t
WHERE t.hometown = 'Blue city' AND c.owner_id = t.id
GROUP BY t.name
)AS T1
WHERE T1.var1 = (
  SELECT MAX(var2)
  FROM (
    SELECT t.name, AVG(c.level) var2
    FROM CatchedPokemon c, Trainer t
    WHERE t.hometown = 'Blue city' AND c.owner_id = t.id
    GROUP BY t.name
  )AS T2
);

SELECT p.name
FROM CatchedPokemon cp
JOIN Pokemon p ON p.id = cp.pid
JOIN Trainer t ON t.id = cp.owner_id
JOIN Evolution e ON e.before_id = p.id
WHERE t.hometown IN(
  SELECT City.name
  FROM City
  JOIN Trainer t ON t.hometown = City.name
  GROUP BY City.name
  HAVING COUNT(*) < 2
)
AND p.type = 'Electric'
ORDER BY t.hometown;

SELECT t.name, SUM(cp.level) level_sum
FROM Gym g
JOIN Trainer t ON t.id = g.leader_id
JOIN CatchedPokemon cp ON cp.owner_id = t.id
GROUP BY t.name
ORDER BY level_sum DESC;

SELECT T1.hometown
FROM (
  SELECT t.hometown, COUNT(*)
  FROM Trainer t
  GROUP BY t.hometown
  ORDER BY COUNT(*) DESC LIMIT 1
  ) AS T1;

SELECT DISTINCT p.name
FROM Pokemon p
WHERE p.id IN (
  SELECT cp1.pid
  FROM CatchedPokemon cp1
  JOIN Trainer t1 ON t1.id = cp1.owner_id
  WHERE t1.hometown = 'Sangnok City'
  )
AND p.id IN (
  SELECT cp2.pid
  FROM CatchedPokemon cp2
  JOIN Trainer t2 ON t2.id = cp2.owner_id
  WHERE t2.hometown = 'Brown City'
  )
ORDER BY p.name;

SELECT t.name
FROM Trainer t
JOIN CatchedPokemon cp ON t.id = cp.owner_id
JOIN Pokemon p ON cp.pid = p.id
WHERE p.name LIKE 'P%' AND t.hometown = 'Sangnok City'
ORDER BY t.name;

SELECT t.name, p.name
FROM Trainer t
JOIN CatchedPokemon cp ON t.id = cp.owner_id
JOIN Pokemon p ON cp.pid = p.id
ORDER BY t.name, p.name;

SELECT p.name
FROM Pokemon p
JOIN Evolution e ON p.id = e.before_id
WHERE p.id NOT IN (
  SELECT e1.after_id
  FROM Evolution e1
) AND p.id NOT IN (
  SELECT e2.before_id
  FROM Evolution e2
  JOIN Evolution e3 ON e2.after_id = e3.before_id
)
ORDER BY p.name;

SELECT cp.nickname
FROM CatchedPokemon cp
JOIN Gym g ON cp.owner_id = g.leader_id
JOIN Pokemon p ON cp.pid = p.id
WHERE g.city = 'Sangnok City' AND p.type = 'Water'
ORDER BY cp.nickname;

SELECT t.name
FROM CatchedPokemon cp
JOIN Trainer t ON cp.owner_id = t.id
JOIN Evolution e ON cp.pid = e.after_id
GROUP BY t.id
HAVING COUNT(*) >= 3
ORDER BY t.name;

SELECT p.name
FROM Pokemon p
WHERE p.id NOT IN (
  SELECT cp.pid
  FROM CatchedPokemon cp
)
ORDER BY p.name;

SELECT MAX(cp.level) max_level
FROM CatchedPokemon cp
JOIN Trainer t ON cp.owner_id = t.id
GROUP BY t.hometown
ORDER BY max_level DESC;

SELECT p1.id, p1.name, p2.name, p3.name
FROM Pokemon p1
JOIN Evolution e1 ON p1.id = e1.before_id
JOIN Pokemon p2 ON e1.after_id = p2.id
JOIN Evolution e2 ON p2.id = e2.before_id
JOIN Pokemon p3 ON e2.after_id = p3.id
ORDER BY p1.id;