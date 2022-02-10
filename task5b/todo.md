#1
Väldigt mycket onödiga debug-utskrifter.

#2
Saknas kontroll för om en mapp är full. Programmet kraschar när man försöker lägga till för många filer.

#4
Saknas kontroll för långa filnamn. Nu kapas bara slutet av vilket gör att man kan skapa flera filer med samma namn genom att göra dem tillräckligt långa.
Detta gäller för create, cp, mv och mkdir.

#5
append mellan två stora filer (fler än ett block) fungerar inte korrekt.
Det som ligger på det sista blocket i source verkar skrivas med en gång extra.
Resultatet blir också konstigt när man gör append på två mindre filer (får plats på ett block) som tillsammans blir en stor fil.
