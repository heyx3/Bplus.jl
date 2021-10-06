# Helpers for writing/debugging macros.

"Prints an expression tree in detail."
print_expr(x, tabbing="") = print_expr(stdout, x, tabbing)
export print_expr

function print_expr(io::IO, x::Expr, tabbing::String="")
    print(io, "Expr ", x.head, " [ ")
    for arg in x.args
        print(io, "\n\t", tabbing)
        print_expr(io, arg, tabbing*"\t")
    end
    if !isempty(x.args)
        print(io, "\n", tabbing)
    end
    print(io, "]")
end
print_expr(io::IO, x::Number, _=nothing) = print(io, x)
print_expr(io::IO, x::Symbol, _=nothing) = print(io, ":(", x, ")")
print_expr(io::IO, x::AbstractString, _=nothing) = print(io, '"', x, '"')
print_expr(io::IO, x::LineNumberNode, _=nothing) = print(io, "[LineNumberNode]")
print_expr(io::IO, x, _=nothing) = print(io, "Unknown expr: ", x)