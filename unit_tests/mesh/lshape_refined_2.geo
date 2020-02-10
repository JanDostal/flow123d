fine_step = 1e-6;
mesh = 0.018;
Point(1) = {0, 0, 0, mesh};
Point(2) = {2, 0, 0, mesh};
Point(3) = {2, 1, 0, mesh};
Point(4) = {1, 1, 0, fine_step};
Point(5) = {1, 2, 0, mesh};
Point(6) = {0, 2, 0, mesh};
Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 5};
Line(5) = {5, 6};
Line(6) = {6, 1};
Line Loop(7) = {1, 2, 3, 4, 5, 6};
Plane Surface(7) = {7};
Physical Line(".boundary") = {1, 2, 3, 4, 5, 6};
Physical Surface("plane") = {7};
