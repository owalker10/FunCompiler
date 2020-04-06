neg1 = 18446744073709551615

print argc
argc = format
print argc
argc = fun { print main }
argc()
print (argc + argc*neg1 == if1 == 0 == end == 1)

print printf
printf = neg1
neg1 = printf
print (printf == neg1)

main = fun {
    while ((c == 100) == 0){
        c = c + 2
        c = c + neg1
    }
    main = main
    inner = fun print 42
    if (4*(inner == inner) == 4)
        end = fun print 666
    else
        end = fun print 999
}

main()
inner()
end()

print (main == inner)

c = 0

f1 = fun {
    if (c == 1000000){
        print 80085
        print c
    }
    else
        f2()
    }
f2 = fun {
    c = c + 1
    f1()
    }

f1()
