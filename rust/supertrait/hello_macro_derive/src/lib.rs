extern crate proc_macro;
use proc_macro::TokenStream;
use quote::quote; // 解析语法树 生成rust代码
use syn; // 解析语法树

// some_attribute 指定过程宏的类型占位符 这里就是HelloMacro
// 接受一个TokenStream 作为输入  并产生一个TokenSream作为输出 其表示一段标记序列
#[proc_macro_derive(HelloMacro)]
pub fn hello_macro_derive(input: TokenStream) -> TokenStream {
    let ast = syn::parse(input).unwrap();

    impl_hello_macro(&ast)
}

fn impl_hello_macro(ast: &syn::DeriveInput) -> TokenStream {
    // 这里获得的是 Pancakes
    let name = &ast.ident;

    let gen = quote! {
        impl HelloMacro for #name {
            fn hello_macro() {
                // stringify 将一个表达式转换为字面值 比如 1+2 就变为"1+2"
                // 这里就是吧#name表达式变为自己Pancakes
                println!("Hello, Macro! My name is {}", stringify!(#name));
            }
        }
    };
    // into方法转换为TokenStream
    gen.into()
}
#[cfg(test)]
mod tests {
    #[derive(HelloMacro)]
    struct Pancakes;
    #[test]
    fn it_works() {
        Pancakes::hello_macro();
    }
}
