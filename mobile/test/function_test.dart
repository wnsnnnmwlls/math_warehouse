import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:math_paper_mobile/math/expression.dart';
import 'package:math_paper_mobile/math/function_graph_page.dart';

void main() {
  test('accepts y= and f(x)= prefixes', () {
    expect(MathExpression.parse('y=x').evaluate(3), 3);
    expect(MathExpression.parse('f(x)=x^2-1').evaluate(3), 8);
    expect(MathExpression.parse('y = 2x + 1').evaluate(2), 5);
  });

  test('supports implicit multiplication and functions', () {
    expect(MathExpression.parse('2x+1').evaluate(2), 5);
    expect(MathExpression.parse('sin(x)').evaluate(math.pi / 2), closeTo(1, 1e-9));
    expect(MathExpression.parse('sqrt(x+1)').evaluate(3), closeTo(2, 1e-9));
    expect(MathExpression.parse('x²-1').evaluate(3), 8);
  });

  testWidgets('function page fits a phone width', (tester) async {
    tester.view.physicalSize = const Size(390, 844);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    await tester.pumpWidget(const MaterialApp(home: FunctionGraphPage()));
    await tester.pump();
    expect(find.text('函数表达式 / 中文描述'), findsOneWidget);
  });
}
