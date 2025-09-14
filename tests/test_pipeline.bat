@echo off
echo ========================================
echo SchismC Complete Pipeline Test Suite
echo ========================================
echo.

echo Testing Literals...
bin\schismc.exe tests\hello_world.hc
echo.

echo Testing Multiple Literals...
bin\schismc.exe tests\parser_test.hc
echo.

echo Testing Various Literal Types...
bin\schismc.exe tests\test_simple_literals.hc
echo.

echo Testing Empty File...
bin\schismc.exe tests\test_empty.hc
echo.

echo Testing Whitespace File...
bin\schismc.exe tests\test_whitespace.hc
echo.

echo Testing Expressions (Expected to fail - not implemented yet)...
bin\schismc.exe tests\test_simple_expressions.hc
echo.

echo Testing Variables (Expected to fail - not implemented yet)...
bin\schismc.exe tests\test_simple_variables.hc
echo.

echo Testing Invalid Tokens (Expected to fail - error handling)...
bin\schismc.exe tests\test_invalid.hc
echo.

echo Testing Missing Semicolon (Expected to fail - syntax error)...
bin\schismc.exe tests\test_missing_semicolon.hc
echo.

echo ========================================
echo Pipeline Test Suite Complete
echo ========================================
