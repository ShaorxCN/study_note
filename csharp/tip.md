
- [类相关](#类相关)
  - [new override：](#new-override)
  - [深度复制](#深度复制)
  - [IDispose](#idispose)
    - [使用 IDisposable 的最佳实践](#使用-idisposable-的最佳实践)
  - [as](#as)
  - [?](#)
- [集合](#集合)
  - [CollectionBase](#collectionbase)
    - [内部结构:](#内部结构)
  - [index运算符重载](#index运算符重载)
  - [比较](#比较)


# 类相关
abstract 只能继承 不能实例化

sealed 只能实例化 不能继承

readonly的成员 说明只能在声明或者构造函数中初始化

写法上 先继承基类 然后实现接口 。可以理解成先读取基类继承的方法才能判断是否实现了接口的规范。

## new override：
- override 多态 用于重写virtual 和 abstract的方法。和实际对象类型关联。重写的方法中可以通过base.xxxx()来调用基类的中的xxxx方法。
- new 隐藏基类的实现，可以理解成实际方法是 类型.xxx()这里基类和子类中的方法实际相当于两个函数名 和类型声明相关。所以这样如果口头调到基类的方法a中 如果a方法调用了b方法。也是直接调用的基类的b方法。


## 深度复制  
实现ICloneable 然后方法是 public object Clone()


## IDispose

```c#
public interface IDisposable
{
    void Dispose();
}
```

1. Dispose 方法:Dispose 方法是显式调用的，用于释放托管和非托管资源。调用 Dispose 后，对象应当不再使用。
2. Dispose(bool disposing) 方法:该方法是实际的清理逻辑所在。
disposing 参数指示是否同时释放托管资源（true gc管理）还是仅释放非托管资源（false）。通常，disposing 为 true 时，释放托管资源；为 false 时，仅释放非托管资源。
3. GC.SuppressFinalize(this):调用 GC.SuppressFinalize(this) 防止垃圾回收器在对象被回收时调用析构函数。这样可以避免不必要的资源释放。
4. 析构函数（Finalizer）:析构函数（~MyResource()）在垃圾回收时被调用，用于确保即使 Dispose 方法未被显式调用，也能释放非托管资源。通常情况下，如果 Dispose 已经被调用，析构函数不应再释放资源，因此 Dispose(false) 被调用。析构函数自然也是释放的非托管资源

### 使用 IDisposable 的最佳实践

使用 using 语句:

using 语句可以自动调用 Dispose 方法，简化资源管理，并减少错误。
```csharp
复制代码
using (var resource = new MyResource())
{
    // 使用资源
} // 自动调用 resource.Dispose()

```

可以通过这个实现类似go的defer 

确保线程安全:

如果你的类是线程安全的，请确保 Dispose 方法也同样线程安全。
避免在析构函数中进行昂贵操作:

析构函数中的资源释放通常较慢，因此应尽量在 Dispose 方法中处理资源释放


## as

类型转换 如果失败不会异常 而是返回null

## ?

这个也是允许为空的作用 

```c#
public class Person
{
    public string Name { get; set; }
}

public class Program
{
    public static void Main()
    {
        Person person = null;
        int? nameLength = person?.Name?.Length;
        Console.WriteLine(nameLength); // 输出: 
    }
}
```

这里如果person等为空不会异常 而是返回null 同样 变量的nameLength允许为null


```c#
var result = possiblyNullValue ?? defaultValue;
```

这里如果possiblyNullValue 为空则赋值 defaultValue


# 集合 
- IEnumerable 可以迭代集合的项目 public IEnumerator GetEnumerator() 实现该方法实现迭代 实现foreach 可以迭代类
- ICollection 继承于IEnumerable 可以获取集合项的个数（Count()） 并且可以把项目复制到一个简单的数组中 提供了Add 和 Remove
- IList 继承于IEnumerable 和 ICollection 提供了集合的项列表 可以访问并且一些基础功能 可以通过index访问
- IDicionary 继承于IEnumerable 和 ICollection 类似IList 但是可以通过key访问value

IEnumerable 中 GetEnumerator 返回 IEnumerator 
其中 IEnumerator 中定义了

```c#
bool MoveNext();
object Current { get; }
void Reset();
```

foreach 先测试`MoveNext()` 如果返回true则通过Current获取当前对象。

任何实现了 IEnumerable 接口的集合都可以使用 foreach 语句进行遍历

yield  return 是返回对象的语句 其中是中断语句 foreach 每次遇到yield就代表返回一个元素并且记录当前状态。等待foreach下次的调用。这样就不用实现上面的MoveNext等了。

编译器会根据yield return 和  yield break 自动生成IEnumerator的实现。yield break自然是提前终止迭代。

##  CollectionBase


CollectionBase 继承自 System.Object，并实现了 ICollection、IEnumerable 和 IList 接口。是一个抽象类，所以不能直接实例化，而是需要通过继承来创建具体的集合类。


### 内部结构:

CollectionBase 维护了一个受保护的 List 属性，这个属性是 ArrayList 类型，用于存储集合中的元素。
子类可以通过 List 属性直接访问和操作存储在集合中的数据。
关键方法和属性:

Count: 返回集合中的元素数量。
List: 受保护的属性，返回一个内部的 ArrayList 实例，子类可以使用这个 ArrayList 来管理集合中的元素。
OnInsert, OnRemove, OnClear: 这些是受保护的虚方法（virtual methods），允许子类在插入、删除或清空集合时执行自定义逻辑。
OnInsertComplete, OnRemoveComplete, OnClearComplete: 与上面的虚方法类似，这些方法在插入、删除或清空操作完成后被调用，用于实现后处理逻辑。


## index运算符重载

```c#
public class MyClass
{
    private int[] data = new int[10];

    // 定义一个索引器
    public int this[int index]
    {
        get
        {
            if (index >= 0 && index < data.Length)
                return data[index];
            else
                throw new IndexOutOfRangeException("Index out of range");
        }
        set
        {
            if (index >= 0 && index < data.Length)
                data[index] = value;
            else
                throw new IndexOutOfRangeException("Index out of range");
        }
    }
}

public class Program
{
    public static void Main()
    {
        MyClass obj = new MyClass();
        obj[0] = 10;
        obj[1] = 20;
        Console.WriteLine(obj[0]); // 输出: 10
        Console.WriteLine(obj[1]); // 输出: 20
    }
}

```

## 比较


instance.GetType() == typof(Class)

一个运行时 一个编译时

指类型的装箱以及拆箱。装箱时复制而不是源的引用 会在堆上重新开辟然后复制获取。而引用类型的装箱 因为他本身已经时引用类型  自然复制的地址仍然时原值。其实也谈不上装箱 实际时类型转换？有点类似c  字面量的数组和指针 一个在栈上开辟然后复制 一个时指针指过去.







