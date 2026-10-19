#include "seed.h"

#include "util.h"

#include <string>
#include <utility>
#include <vector>

namespace {

Question Make(Difficulty difficulty,
              QuestionType type,
              std::string stem,
              std::string answer,
              std::string analysis,
              std::vector<std::string> formulas,
              std::vector<std::string> concepts,
              std::vector<std::string> options = {}) {
    Question question;
    question.id = util::NewId("seed");
    question.difficulty = difficulty;
    question.type = type;
    question.stem = std::move(stem);
    question.options = std::move(options);
    question.answer = std::move(answer);
    question.analysis = std::move(analysis);
    question.formulas = std::move(formulas);
    question.concepts = std::move(concepts);
    question.tagged = question.HasAnyTag();
    question.created = util::NowIso8601();
    return question;
}

}  // namespace

std::vector<Question> BuildSeedQuestions() {
    std::vector<Question> questions;
    questions.reserve(24);

    questions.push_back(Make(
        Difficulty::A, QuestionType::Choice,
        "已知集合 A={1,2,3}，B={2,3,4}，则 A∩B=（　　）",
        "B",
        "集合 A 与 B 的公共元素为 2 和 3，因此 A∩B={2,3}。",
        {"集合交集的定义"},
        {"集合", "集合的交集", "集合的表示法"},
        {"{1,2}", "{2,3}", "{3,4}", "{1,4}"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Choice,
        "已知函数 f(x)=2x+1，则 f(2)=（　　）",
        "C",
        "将 x=2 代入解析式，得 f(2)=2×2+1=5。",
        {"一次函数解析式"},
        {"函数", "函数求值", "一次函数"},
        {"3", "4", "5", "6"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Fill,
        "sin 30° 的值为______．",
        "1/2",
        "sin 30°=1/2，属于特殊角的三角函数值。",
        {"特殊角三角函数值"},
        {"三角函数", "任意角的三角函数"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Fill,
        "已知等差数列 {aₙ} 中，a₁=2，公差 d=3，则 a₅=______．",
        "14",
        "由等差数列通项公式，a₅=a₁+4d=2+4×3=14。",
        {"等差数列通项公式"},
        {"等差数列", "数列的通项公式"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Fill,
        "若复数 z=3+4i，则 |z|=______．",
        "5",
        "由复数模的公式，|z|=√(3²+4²)=5。",
        {"复数模的公式"},
        {"复数的概念", "复平面"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Choice,
        "抛掷一枚质地均匀的骰子一次，出现偶数点的概率为（　　）",
        "C",
        "样本空间共 6 个等可能结果，偶数点有 2、4、6 共 3 个，故概率为 3/6=1/2。",
        {"古典概型概率公式"},
        {"古典概型", "样本空间"},
        {"1/6", "1/3", "1/2", "2/3"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Fill,
        "已知正方体的棱长为 2，则该正方体的体积为______．",
        "8",
        "由正方体体积公式，V=a³=2³=8。",
        {"正方体体积公式"},
        {"立体几何", "正方体", "空间几何体的体积"}));

    questions.push_back(Make(
        Difficulty::A, QuestionType::Choice,
        "已知向量 a=(1,2)，b=(3,-1)，则 a·b=（　　）",
        "B",
        "由平面向量数量积的坐标公式，a·b=1×3+2×(-1)=1。",
        {"平面向量数量积的坐标公式"},
        {"平面向量", "数量积"},
        {"-1", "1", "5", "7"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Choice,
        "函数 f(x)=x²-4x+3 的单调递减区间是（　　）",
        "A",
        "f'(x)=2x-4。令 f'(x)≤0，得 x≤2，故函数在 (-∞,2] 上单调递减。",
        {"导数与函数单调性的关系", "二次函数对称轴公式"},
        {"函数", "二次函数", "函数的单调性"},
        {"(-∞,2]", "[2,+∞)", "(-∞,-2]", "[-2,+∞)"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Choice,
        "在△ABC 中，a=3，b=4，C=60°，则 c=（　　）",
        "A",
        "由余弦定理，c²=a²+b²-2ab cos C=9+16-2×3×4×1/2=13，所以 c=√13。",
        {"余弦定理"},
        {"三角函数", "解三角形", "三角形的边角关系"},
        {"√13", "√37", "5", "7"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Fill,
        "已知 sin α=3/5，且 α∈(π/2,π)，则 cos α=______．",
        "-4/5",
        "因为 α 在第二象限，所以 cos α<0。由 sin²α+cos²α=1，得 cos α=-√(1-(3/5)²)=-4/5。",
        {"同角三角函数的基本关系"},
        {"三角函数", "三角函数值的符号", "同角三角函数关系"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Fill,
        "数列 {aₙ} 满足 a₁=1，aₙ₊₁=2aₙ+1，则 a₄=______．",
        "15",
        "由 an+1=2an+1 得 an+1+1=2(an+1)，所以 {an+1} 是以 2 为首项、2 为公比的等比数列，an+1=2ⁿ，即 an=2ⁿ-1，故 a₄=15。",
        {"构造等比数列求通项"},
        {"递推数列", "等比数列"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Solution,
        "已知圆 C 的方程为 x²+y²-4x-6y+4=0，求圆 C 的圆心坐标和半径．",
        "圆心为 (2,3)，半径为 3．",
        "将圆的一般方程配方，得 (x-2)²+(y-3)²=9，因此圆心坐标为 (2,3)，半径 r=3。",
        {"圆的一般方程化为标准方程"},
        {"圆的标准方程", "配方法"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Fill,
        "将 4 本不同的书分给 3 名同学，每人至少分得 1 本，则不同的分法共有______种．",
        "36",
        "先从 4 本书中选出 2 本作为一组，有 C(4,2)=6 种；再将 3 组书分给 3 名同学，有 3!=6 种。所以共有 6×6=36 种不同的分法。",
        {"组合数公式", "排列数公式"},
        {"分组分配问题", "两个计数原理"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Choice,
        "已知 log₂x<1，则实数 x 的取值范围是（　　）",
        "A",
        "由 log₂x<log₂2，且 y=log₂x 在 (0,+∞) 上单调递增，得 0<x<2。",
        {},
        {"对数不等式", "对数函数的单调性", "对数函数的定义域"},
        {"(0,2)", "(0,1)", "(1,2)", "(2,+∞)"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Choice,
        "棱长为 2 的正方体的外接球的表面积为（　　）",
        "C",
        "正方体的体对角线长为 2√3，即外接球的直径，所以外接球半径 R=√3，表面积 S=4πR²=12π。",
        {"球表面积公式", "长方体体对角线公式"},
        {"立体几何", "正方体的外接球", "空间几何体的结构"},
        {"4π", "8π", "12π", "16π"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Fill,
        "函数 f(x)=x³-3x 在区间 [-2,2] 上的最大值为______．",
        "2",
        "f'(x)=3x²-3=3(x-1)(x+1)。比较极值点与端点函数值：f(-2)=-2，f(-1)=2，f(1)=-2，f(2)=2，所以最大值为 2。",
        {"导数与极值的关系"},
        {"闭区间上的最值", "利用导数研究函数性质"}));

    questions.push_back(Make(
        Difficulty::B, QuestionType::Solution,
        "已知数列 {aₙ} 的前 n 项和 Sₙ=n²+n．(1) 求 {aₙ} 的通项公式；(2) 设 bₙ=1/(aₙaₙ₊₁)，求数列 {bₙ} 的前 n 项和 Tₙ．",
        "(1) aₙ=2n（n∈N*）；(2) Tₙ=n/[4(n+1)]．",
        "(1) 当 n=1 时，a₁=S₁=2；当 n≥2 时，aₙ=Sₙ-Sₙ₋₁=n²+n-[(n-1)²+(n-1)]=2n。n=1 时也成立，故 aₙ=2n。\n(2) bₙ=1/(2n·2(n+1))=1/4·[1/n-1/(n+1)]，所以 Tₙ=1/4·[(1-1/2)+(1/2-1/3)+…+(1/n-1/(n+1))]=n/[4(n+1)]。",
        {"aₙ与Sₙ的关系", "裂项相消法"},
        {"数列的前n项和", "裂项相消求和"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Choice,
        "曲线 y=xeˣ 在 x=0 处的切线方程为（　　）",
        "A",
        "y'=eˣ+xeˣ。当 x=0 时，y=0，y'=1，所以切线方程为 y=x。",
        {"导数的几何意义", "乘积的求导法则"},
        {"切线方程", "导数的计算"},
        {"y=x", "y=2x", "y=-x", "y=x+1"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Fill,
        "已知 x>0，y>0，且 x+y=1，则 1/x+4/y 的最小值为______．",
        "9",
        "1/x+4/y=(x+y)(1/x+4/y)=5+y/x+4x/y≥5+2√(y/x·4x/y)=9，当且仅当 y=2x，即 x=1/3、y=2/3 时取等号。",
        {"基本不等式"},
        {"最值问题", "代换法"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Fill,
        "双曲线 x²/4-y²/12=1 的离心率为______．",
        "2",
        "由双曲线方程得 a²=4，b²=12，所以 c²=a²+b²=16，c=4，离心率 e=c/a=2。",
        {"双曲线的离心率公式"},
        {"双曲线的标准方程", "双曲线的性质"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Solution,
        "已知抛物线 y²=4x 的焦点为 F，过 F 的直线与抛物线交于 A、B 两点，若线段 AB 中点的横坐标为 3，求 |AB|．",
        "|AB|=8．",
        "抛物线 y²=4x 的准线为 x=-1，焦点 F(1,0)。设直线 AB 的方程为 x=my+1，与 y²=4x 联立得 y²-4my-4=0。设 A(x₁,y₁)，B(x₂,y₂)，则 y₁+y₂=4m，x₁+x₂=m(y₁+y₂)+2=4m²+2。由中点横坐标为 3，得 (x₁+x₂)/2=3，所以 4m²+2=6，m²=1。由抛物线焦点弦长公式，|AB|=x₁+x₂+p=6+2=8。",
        {"抛物线的焦点弦长公式", "根与系数的关系"},
        {"直线与抛物线的位置关系", "焦点弦"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Solution,
        "在三棱锥 P-ABC 中，PA⊥平面 ABC，AB⊥AC，PA=AB=AC=2，求三棱锥 P-ABC 外接球的表面积．",
        "12π．",
        "因为 PA、AB、AC 两两垂直且长度均为 2，可将三棱锥补形成棱长为 2 的正方体，三棱锥的外接球即该正方体的外接球。正方体的体对角线长为 2√3，所以外接球半径 R=√3，表面积 S=4πR²=12π。",
        {"球表面积公式", "长方体体对角线公式"},
        {"立体几何", "补形法", "三棱锥的外接球"}));

    questions.push_back(Make(
        Difficulty::C, QuestionType::Solution,
        "已知函数 f(x)=ln x-ax+1（a∈R），求函数 f(x) 的单调区间．",
        "当 a≤0 时，f(x) 在 (0,+∞) 上单调递增；当 a>0 时，f(x) 在 (0,1/a) 上单调递增，在 (1/a,+∞) 上单调递减．",
        "函数定义域为 (0,+∞)，f'(x)=1/x-a=(1-ax)/x。\n当 a≤0 时，f'(x)>0 恒成立，故 f(x) 在 (0,+∞) 上单调递增。\n当 a>0 时，令 f'(x)>0 得 0<x<1/a；令 f'(x)<0 得 x>1/a。故 f(x) 在 (0,1/a) 上单调递增，在 (1/a,+∞) 上单调递减。",
        {"导数与函数单调性的关系"},
        {"函数", "含参数分类讨论", "利用导数求单调区间"}));

    return questions;
}
